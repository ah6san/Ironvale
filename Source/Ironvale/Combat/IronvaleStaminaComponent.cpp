// =============================================================================
// IronvaleStaminaComponent.cpp — Stamina system implementation
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleStaminaComponent.h"
#include "Ironvale.h"

UIronvaleStaminaComponent::UIronvaleStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIronvaleStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentStamina = MaxStamina;
}

void UIronvaleStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Count down regen pause
	if (RegenPauseRemaining > 0.0f)
	{
		RegenPauseRemaining -= DeltaTime;
		return;
	}

	// Regenerate stamina
	if (CurrentStamina < MaxStamina)
	{
		const float EffectiveRegenRate = BaseRegenRate * (1.0f - ArmorRegenPenalty) * RegenModifier;
		const float RegenAmount = EffectiveRegenRate * DeltaTime;

		const float OldStamina = CurrentStamina;
		CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + RegenAmount);

		if (CurrentStamina != OldStamina)
		{
			OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, CurrentStamina - OldStamina);
		}

		// Clear depleted state once we've regenerated above 0
		if (bWasDepleted && CurrentStamina > 0.0f)
		{
			bWasDepleted = false;
		}
	}
}

bool UIronvaleStaminaComponent::ConsumeStamina(float Amount, bool bForceConsume)
{
	if (Amount <= 0.0f) return true;

	if (!bForceConsume && CurrentStamina < Amount)
	{
		return false; // Insufficient stamina
	}

	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);

	// Pause regen after consumption
	PauseRegenFor(RegenDelay);

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, CurrentStamina - OldStamina);

	// Check for depletion
	if (CurrentStamina <= 0.0f && !bWasDepleted)
	{
		bWasDepleted = true;
		OnStaminaDepleted.Broadcast();
		UE_LOG(LogIronvale, Verbose, TEXT("%s stamina depleted!"), *GetOwner()->GetName());
	}

	return true;
}

void UIronvaleStaminaComponent::RestoreStamina(float Amount)
{
	if (Amount <= 0.0f) return;

	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + Amount);

	if (CurrentStamina != OldStamina)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, CurrentStamina - OldStamina);
	}
}

void UIronvaleStaminaComponent::PauseRegenFor(float Seconds)
{
	RegenPauseRemaining = FMath::Max(RegenPauseRemaining, Seconds);
}

float UIronvaleStaminaComponent::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? FMath::Max(0.0f, CurrentStamina / MaxStamina) : 0.0f;
}

void UIronvaleStaminaComponent::SetArmorRegenPenalty(float Penalty)
{
	ArmorRegenPenalty = FMath::Clamp(Penalty, 0.0f, 0.9f);
}

void UIronvaleStaminaComponent::SetMaxStamina(float NewMax)
{
	MaxStamina = FMath::Max(1.0f, NewMax);
	CurrentStamina = FMath::Min(CurrentStamina, MaxStamina);
}
