// =============================================================================
// IronvaleCombatComponent.cpp — KCD-style combat state machine implementation
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleCombatComponent.h"
#include "Ironvale.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Combat/IronvaleHealthComponent.h"
#include "Combat/IronvaleDamageCalculator.h"
#include "Inventory/IronvaleEquipmentComponent.h"
#include "Core/IronvaleEventBus.h"
#include "Core/IronvaleStatics.h"

UIronvaleCombatComponent::UIronvaleCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIronvaleCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache sibling component references
	if (AActor* Owner = GetOwner())
	{
		StaminaComp = Owner->FindComponentByClass<UIronvaleStaminaComponent>();
		HealthComp = Owner->FindComponentByClass<UIronvaleHealthComponent>();
		EquipmentComp = Owner->FindComponentByClass<UIronvaleEquipmentComponent>();
	}

	// Bind stamina depletion to stagger
	if (StaminaComp)
	{
		StaminaComp->OnStaminaDepleted.AddDynamic(this, &UIronvaleCombatComponent::HandleStaminaDepleted);
	}
}

void UIronvaleCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCharge(DeltaTime);
	UpdateStagger(DeltaTime);
	UpdateRiposteWindow(DeltaTime);
	ClearExpiredInputs();
}

// =============================================================================
// STATE MACHINE
// =============================================================================

void UIronvaleCombatComponent::TransitionTo(EIronvaleCombatState NewState)
{
	if (CurrentState == NewState) return;
	if (CurrentState == EIronvaleCombatState::Dead) return; // No leaving death

	const EIronvaleCombatState OldState = CurrentState;
	OnExitState(OldState);
	CurrentState = NewState;
	OnEnterState(NewState);

	// Broadcast state change
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnCombatStateChanged.Broadcast(GetOwner(), NewState);
	}

	UE_LOG(LogIronvale, Verbose, TEXT("%s combat: %d -> %d"),
		*GetOwner()->GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void UIronvaleCombatComponent::OnEnterState(EIronvaleCombatState State)
{
	switch (State)
	{
	case EIronvaleCombatState::Staggered:
		StaggerTimer = StaggerDuration;
		break;
	case EIronvaleCombatState::Blocking:
		BlockStartTime = GetWorld()->GetTimeSeconds();
		break;
	default:
		break;
	}
}

void UIronvaleCombatComponent::OnExitState(EIronvaleCombatState State)
{
	switch (State)
	{
	case EIronvaleCombatState::Attacking:
		bIsCharging = false;
		break;
	case EIronvaleCombatState::Blocking:
		CurrentBlockDirection = EIronvaleAttackDirection::None;
		break;
	default:
		break;
	}
}

void UIronvaleCombatComponent::ForceState(EIronvaleCombatState NewState)
{
	CurrentState = NewState;
}

bool UIronvaleCombatComponent::CanAttack() const
{
	return (CurrentState == EIronvaleCombatState::Idle ||
			CurrentState == EIronvaleCombatState::LockedOn ||
			(CurrentState == EIronvaleCombatState::Parrying && bRiposteAvailable));
}

bool UIronvaleCombatComponent::CanBlock() const
{
	return (CurrentState == EIronvaleCombatState::Idle ||
			CurrentState == EIronvaleCombatState::LockedOn);
}

// =============================================================================
// ATTACK
// =============================================================================

void UIronvaleCombatComponent::StartAttack(EIronvaleAttackDirection Direction)
{
	if (!CanAttack()) return;
	if (Direction == EIronvaleAttackDirection::None) return;

	const FIronvaleItemData* WeaponData = GetEquippedWeaponData();
	const float StaminaCost = WeaponData ? WeaponData->StaminaCostPerAttack : 10.0f;

	// Check stamina
	if (StaminaComp && !StaminaComp->HasStamina(StaminaCost))
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Cannot attack: insufficient stamina"));
		return;
	}

	// Check for riposte
	const bool bIsRiposte = (CurrentState == EIronvaleCombatState::Parrying && bRiposteAvailable);

	// Set up attack data
	CurrentAttack = FIronvaleAttackData();
	CurrentAttack.Direction = Direction;
	CurrentAttack.DamageType = WeaponData ? WeaponData->PrimaryDamageType : EIronvaleDamageType::Blunt;
	CurrentAttack.BaseDamage = WeaponData ? WeaponData->BaseDamage : 5.0f;
	CurrentAttack.StaggerPower = WeaponData ? WeaponData->StaggerPower : 5.0f;
	CurrentAttack.StaminaCost = StaminaCost;
	CurrentAttack.bIsRiposte = bIsRiposte;
	CurrentAttack.bIsCommitted = false;
	CurrentAttack.ChargeLevel = 0.0f;
	CurrentAttack.StartTime = GetWorld()->GetTimeSeconds();
	CurrentAttack.Attacker = GetOwner();

	bIsCharging = true;
	ChargeStartTime = GetWorld()->GetTimeSeconds();

	TransitionTo(EIronvaleCombatState::Attacking);

	// Add to input buffer for combo detection
	AddToInputBuffer(Direction, false);
}

void UIronvaleCombatComponent::ReleaseAttack()
{
	if (CurrentState != EIronvaleCombatState::Attacking) return;

	bIsCharging = false;
	CommitAttack();
}

bool UIronvaleCombatComponent::TryFeint()
{
	if (CurrentState != EIronvaleCombatState::Attacking) return false;
	if (CurrentAttack.bIsCommitted) return false; // Past the point of no return

	// Feint costs a fraction of the attack stamina
	if (StaminaComp)
	{
		StaminaComp->ConsumeStamina(CurrentAttack.StaminaCost * 0.3f, true);
	}

	TransitionTo(EIronvaleCombatState::LockedOn);
	UE_LOG(LogIronvale, Verbose, TEXT("Attack feinted"));
	return true;
}

void UIronvaleCombatComponent::UpdateCharge(float DeltaTime)
{
	if (!bIsCharging || CurrentState != EIronvaleCombatState::Attacking) return;

	const float ChargeTime = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	CurrentAttack.ChargeLevel = FMath::Clamp(ChargeTime / MaxChargeTime, 0.0f, 1.0f);

	// Auto-commit after feint window
	if (!CurrentAttack.bIsCommitted && ChargeTime >= FeintCancelWindow)
	{
		CurrentAttack.bIsCommitted = true;
	}

	// Auto-release at max charge
	if (ChargeTime >= MaxChargeTime)
	{
		bIsCharging = false;
		CommitAttack();
	}
}

void UIronvaleCombatComponent::CommitAttack()
{
	CurrentAttack.bIsCommitted = true;

	// Consume stamina
	if (StaminaComp)
	{
		StaminaComp->ConsumeStamina(CurrentAttack.StaminaCost, true);
		StaminaComp->PauseRegenFor(0.5f);
	}

	// Apply charge bonus (up to 50% more damage at full charge)
	CurrentAttack.BaseDamage *= (1.0f + CurrentAttack.ChargeLevel * 0.5f);

	// Riposte bonus
	if (CurrentAttack.bIsRiposte)
	{
		CurrentAttack.BaseDamage *= 1.5f;
	}

	// Check combos
	CheckCombos();

	// NOTE: Actual hit detection happens via weapon trace (AIronvaleWeaponActor)
	// The weapon actor reads CurrentAttack and calls ProcessIncomingHit on targets.
	// After the attack animation completes, an anim notify transitions back to LockedOn/Idle.

	UE_LOG(LogIronvale, Verbose, TEXT("Attack committed: Dir=%d, Damage=%.1f, Charge=%.2f, Riposte=%d"),
		static_cast<int32>(CurrentAttack.Direction),
		CurrentAttack.BaseDamage,
		CurrentAttack.ChargeLevel,
		CurrentAttack.bIsRiposte);
}

// =============================================================================
// BLOCK / PARRY
// =============================================================================

void UIronvaleCombatComponent::StartBlock()
{
	if (!CanBlock()) return;

	if (StaminaComp && StaminaComp->IsDepleted())
	{
		return; // Can't block with no stamina
	}

	TransitionTo(EIronvaleCombatState::Blocking);
}

void UIronvaleCombatComponent::StopBlock()
{
	if (CurrentState != EIronvaleCombatState::Blocking &&
		CurrentState != EIronvaleCombatState::Parrying) return;

	TransitionTo(EIronvaleCombatState::LockedOn);
}

FIronvaleBlockResult UIronvaleCombatComponent::EvaluateBlock(const FIronvaleAttackData& IncomingAttack)
{
	FIronvaleBlockResult Result;

	if (CurrentState != EIronvaleCombatState::Blocking &&
		CurrentState != EIronvaleCombatState::Parrying)
	{
		return Result; // Not blocking
	}

	Result.bBlocked = true;

	// Check for perfect parry (within timing window of block start)
	const float TimeSinceBlockStart = GetWorld()->GetTimeSeconds() - BlockStartTime;
	Result.bPerfectParry = (TimeSinceBlockStart <= ParryWindowDuration);

	// Base block stamina cost (attacker's weapon stagger power)
	Result.StaminaCost = IncomingAttack.StaggerPower;

	if (Result.bPerfectParry)
	{
		// Perfect parry: minimal stamina cost, stagger the attacker
		Result.StaminaCost *= 0.25f;
		Result.StaggerToAttacker = IncomingAttack.StaggerPower * 1.5f;
		Result.ResidualDamage = 0.0f;

		// Enter parry state and open riposte window
		TransitionTo(EIronvaleCombatState::Parrying);
		bRiposteAvailable = true;
		RiposteWindowTimer = RiposteWindowDuration;

		UE_LOG(LogIronvale, Verbose, TEXT("Perfect parry! Riposte window open."));
	}
	else
	{
		// Normal block: full stamina cost, some chip damage
		Result.ResidualDamage = IncomingAttack.BaseDamage * 0.1f; // 10% chip damage
		Result.StaggerToBlocker = IncomingAttack.StaggerPower * 0.5f;
	}

	// Consume block stamina
	if (StaminaComp)
	{
		StaminaComp->ConsumeStamina(Result.StaminaCost, true);
	}

	return Result;
}

// =============================================================================
// HIT PROCESSING
// =============================================================================

FIronvaleDamageResult UIronvaleCombatComponent::ProcessIncomingHit(const FIronvaleHitData& HitData)
{
	FIronvaleDamageResult Result;

	// Check if blocking
	if (CurrentState == EIronvaleCombatState::Blocking || CurrentState == EIronvaleCombatState::Parrying)
	{
		FIronvaleBlockResult BlockResult = EvaluateBlock(HitData.AttackData);
		if (BlockResult.bBlocked)
		{
			Result.FinalDamage = BlockResult.ResidualDamage;
			Result.StaggerDamage = BlockResult.StaggerToBlocker;
			Result.HitZone = HitData.HitZone;
			Result.AttackDirection = HitData.AttackData.Direction;
			Result.DamageType = HitData.AttackData.DamageType;

			// Apply chip damage if any
			if (HealthComp && Result.FinalDamage > 0.0f)
			{
				HealthComp->ApplyDamage(Result.FinalDamage, HitData.AttackData.Attacker.Get());
			}

			// Apply stagger from block
			ApplyStagger(Result.StaggerDamage);

			return Result;
		}
	}

	// Unblocked hit — calculate full damage via DamageCalculator
	TArray<FIronvaleArmorLayer> ArmorLayers;
	if (EquipmentComp)
	{
		ArmorLayers = EquipmentComp->GetArmorLayersForZone(HitData.HitZone);
	}

	Result = UIronvaleDamageCalculator::CalculateDamage(HitData.AttackData, ArmorLayers, HitData.HitZone);

	// Apply damage to health
	if (HealthComp)
	{
		HealthComp->ApplyDamage(Result.FinalDamage, HitData.AttackData.Attacker.Get());
	}

	// Apply stagger
	ApplyStagger(Result.StaggerDamage);

	// Apply durability damage to armor
	if (EquipmentComp)
	{
		EquipmentComp->DamageArmorInZone(HitData.HitZone, Result.ArmorDurabilityDamage);
	}

	// Broadcast damage event
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnDamageDealt.Broadcast(
			HitData.AttackData.Attacker.Get(), GetOwner(), Result);
	}

	return Result;
}

// =============================================================================
// STAGGER
// =============================================================================

void UIronvaleCombatComponent::ApplyStagger(float StaggerAmount)
{
	if (CurrentState == EIronvaleCombatState::Dead) return;

	CurrentStagger += StaggerAmount;

	if (CurrentStagger >= StaggerThreshold)
	{
		CurrentStagger = 0.0f;
		TransitionTo(EIronvaleCombatState::Staggered);
	}
}

void UIronvaleCombatComponent::UpdateStagger(float DeltaTime)
{
	// Recover stagger over time
	if (CurrentStagger > 0.0f && CurrentState != EIronvaleCombatState::Staggered)
	{
		CurrentStagger = FMath::Max(0.0f, CurrentStagger - StaggerRecoveryRate * DeltaTime);
	}

	// Count down stagger timer
	if (CurrentState == EIronvaleCombatState::Staggered)
	{
		StaggerTimer -= DeltaTime;
		if (StaggerTimer <= 0.0f)
		{
			TransitionTo(EIronvaleCombatState::LockedOn);
		}
	}
}

void UIronvaleCombatComponent::UpdateRiposteWindow(float DeltaTime)
{
	if (!bRiposteAvailable) return;

	RiposteWindowTimer -= DeltaTime;
	if (RiposteWindowTimer <= 0.0f)
	{
		bRiposteAvailable = false;
	}
}

// =============================================================================
// INPUT BUFFER & COMBOS
// =============================================================================

void UIronvaleCombatComponent::AddToInputBuffer(EIronvaleAttackDirection Direction, bool bCharged)
{
	FIronvaleCombatInputEntry Entry;
	Entry.Direction = Direction;
	Entry.Timestamp = GetWorld()->GetTimeSeconds();
	Entry.bCharged = bCharged;
	InputBuffer.Add(Entry);
}

void UIronvaleCombatComponent::ClearExpiredInputs()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	InputBuffer.RemoveAll([this, CurrentTime](const FIronvaleCombatInputEntry& Entry)
	{
		return (CurrentTime - Entry.Timestamp) > InputBufferWindow;
	});
}

void UIronvaleCombatComponent::CheckCombos()
{
	if (InputBuffer.Num() < 2 || ComboDefinitions.Num() == 0) return;

	const FIronvaleItemData* WeaponData = GetEquippedWeaponData();
	const EIronvaleWeaponType CurrentWeaponType = WeaponData ? WeaponData->WeaponType : EIronvaleWeaponType::Fists;

	for (const FIronvaleComboDefinition& Combo : ComboDefinitions)
	{
		// Check weapon type compatibility
		if (Combo.ValidWeaponTypes.Num() > 0 && !Combo.ValidWeaponTypes.Contains(CurrentWeaponType))
		{
			continue;
		}

		if (InputBuffer.Num() < Combo.Sequence.Num()) continue;

		// Check if the last N inputs match the combo sequence
		bool bMatch = true;
		const int32 StartIdx = InputBuffer.Num() - Combo.Sequence.Num();
		for (int32 i = 0; i < Combo.Sequence.Num(); ++i)
		{
			if (InputBuffer[StartIdx + i].Direction != Combo.Sequence[i])
			{
				bMatch = false;
				break;
			}

			// Check timing between inputs
			if (i > 0)
			{
				const float TimeBetween = InputBuffer[StartIdx + i].Timestamp - InputBuffer[StartIdx + i - 1].Timestamp;
				if (TimeBetween > Combo.MaxTimeBetweenInputs)
				{
					bMatch = false;
					break;
				}
			}
		}

		if (bMatch)
		{
			// Combo triggered — apply multiplier and extra cost
			CurrentAttack.BaseDamage *= Combo.DamageMultiplier;
			if (StaminaComp)
			{
				StaminaComp->ConsumeStamina(Combo.BonusStaminaCost, true);
			}

			UE_LOG(LogIronvale, Log, TEXT("Combo triggered: %s (%.1fx damage)"),
				*Combo.ComboID.ToString(), Combo.DamageMultiplier);

			// Clear buffer after combo triggers to prevent double-triggering
			InputBuffer.Empty();
			break;
		}
	}
}

// =============================================================================
// HELPERS
// =============================================================================

const FIronvaleItemData* UIronvaleCombatComponent::GetEquippedWeaponData() const
{
	if (EquipmentComp)
	{
		if (UIronvaleItemInstance* Weapon = EquipmentComp->GetMainHandWeapon())
		{
			return &Weapon->GetItemData();
		}
	}
	return nullptr;
}

void UIronvaleCombatComponent::HandleStaminaDepleted()
{
	ApplyStagger(StaggerThreshold);
}
