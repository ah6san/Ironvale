// =============================================================================
// IronvaleCrimeResponseComponent.cpp — Crime witness and response implementation
// Project Ironvale
// =============================================================================

#include "AI/IronvaleCrimeResponseComponent.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "Characters/IronvaleNPCCharacter.h"
#include "AI/IronvaleScheduleComponent.h"
#include "AI/IronvaleScheduleTypes.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"

UIronvaleCrimeResponseComponent::UIronvaleCrimeResponseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void UIronvaleCrimeResponseComponent::BeginPlay()
{
	Super::BeginPlay();

	// Subscribe to global crime events
	if (UWorld* World = GetWorld())
	{
		if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnCrimeCommitted.AddDynamic(this, &UIronvaleCrimeResponseComponent::HandleCrimeCommitted);
		}
	}

	UE_LOG(LogIronvale, Verbose, TEXT("[CrimeResponse] %s initialized (Range=%.0f, LOS=%s)"),
		*GetOwner()->GetName(),
		WitnessRange,
		bRequireLineOfSight ? TEXT("Yes") : TEXT("No"));
}

void UIronvaleCrimeResponseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from EventBus
	if (UWorld* World = GetWorld())
	{
		if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnCrimeCommitted.RemoveDynamic(this, &UIronvaleCrimeResponseComponent::HandleCrimeCommitted);
		}
	}

	// Clear any active timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CrimeMemoryTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

EIronvaleCrimeResponse UIronvaleCrimeResponseComponent::GetResponseType() const
{
	const AIronvaleNPCCharacter* NPC = GetOwnerNPC();
	if (!NPC)
	{
		return EIronvaleCrimeResponse::None;
	}

	return DetermineResponse(NPC->Archetype);
}

void UIronvaleCrimeResponseComponent::WitnessCrime(AActor* Criminal, FName CrimeType)
{
	if (!Criminal)
	{
		return;
	}

	const AIronvaleNPCCharacter* NPC = GetOwnerNPC();
	if (!NPC)
	{
		return;
	}

	// Do not re-witness while already tracking the same criminal for the same crime
	if (bHasWitnessedCrime && CriminalActor == Criminal && WitnessedCrimeType == CrimeType)
	{
		return;
	}

	bHasWitnessedCrime = true;
	WitnessedCrimeType = CrimeType;
	CriminalActor = Criminal;

	const EIronvaleCrimeResponse Response = GetResponseType();

	UE_LOG(LogIronvale, Log, TEXT("[CrimeResponse] %s witnessed crime '%s' by %s -> Response: %d"),
		*GetOwner()->GetName(),
		*CrimeType.ToString(),
		*Criminal->GetName(),
		static_cast<int32>(Response));

	// Broadcast local event
	OnCrimeWitnessed.Broadcast(Criminal, CrimeType, Response);

	// Apply archetype-specific immediate reactions
	switch (Response)
	{
	case EIronvaleCrimeResponse::Pursue:
		// Guards: inject a combat/pursue override into the schedule
		HandleGuardPursue(Criminal);
		break;

	case EIronvaleCrimeResponse::Flee:
		// Merchants, children, travelers: inject a flee override
		HandleFlee();
		break;

	case EIronvaleCrimeResponse::Report:
		// Villagers, nobles, priests: try to find a guard
		ReportCrime();
		break;

	case EIronvaleCrimeResponse::Ignore:
	case EIronvaleCrimeResponse::None:
	default:
		break;
	}

	// Start memory expiration timer
	if (CrimeMemoryDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CrimeMemoryTimerHandle);
			World->GetTimerManager().SetTimer(
				CrimeMemoryTimerHandle,
				FTimerDelegate::CreateUObject(this, &UIronvaleCrimeResponseComponent::HandleCrimeMemoryExpired),
				CrimeMemoryDuration,
				false);
		}
	}
}

bool UIronvaleCrimeResponseComponent::ReportCrime()
{
	if (!bHasWitnessedCrime || !CriminalActor.IsValid())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Find the nearest guard NPC
	AIronvaleNPCCharacter* NearestGuard = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	const FVector OwnerLocation = Owner->GetActorLocation();

	for (TActorIterator<AIronvaleNPCCharacter> It(World); It; ++It)
	{
		AIronvaleNPCCharacter* OtherNPC = *It;
		if (!OtherNPC || OtherNPC == Owner || OtherNPC->IsDead())
		{
			continue;
		}

		if (OtherNPC->Archetype != EIronvaleNPCArchetype::Guard)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerLocation, OtherNPC->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestGuard = OtherNPC;
		}
	}

	if (!NearestGuard)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("[CrimeResponse] %s cannot report crime — no guard found"),
			*Owner->GetName());
		return false;
	}

	// Notify the guard's crime response component about the criminal
	if (UIronvaleCrimeResponseComponent* GuardCrimeComp = NearestGuard->GetCrimeResponseComponent())
	{
		GuardCrimeComp->WitnessCrime(CriminalActor.Get(), WitnessedCrimeType);

		UE_LOG(LogIronvale, Log, TEXT("[CrimeResponse] %s reported crime '%s' to guard %s"),
			*Owner->GetName(),
			*WitnessedCrimeType.ToString(),
			*NearestGuard->GetName());

		return true;
	}

	return false;
}

void UIronvaleCrimeResponseComponent::ForgetCrime()
{
	bHasWitnessedCrime = false;
	WitnessedCrimeType = NAME_None;
	CriminalActor.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CrimeMemoryTimerHandle);
	}

	// Clear any crime-related schedule overrides
	if (AIronvaleNPCCharacter* NPC = GetOwnerNPC())
	{
		if (UIronvaleScheduleComponent* ScheduleComp = NPC->GetScheduleComponent())
		{
			ScheduleComp->ClearOverride(TEXT("CrimeResponse_Pursue"));
			ScheduleComp->ClearOverride(TEXT("CrimeResponse_Flee"));
		}
	}

	UE_LOG(LogIronvale, Verbose, TEXT("[CrimeResponse] %s forgot witnessed crime"),
		*GetOwner()->GetName());
}

// -----------------------------------------------------------------------------
// EventBus handler
// -----------------------------------------------------------------------------

void UIronvaleCrimeResponseComponent::HandleCrimeCommitted(
	AActor* Criminal, FName CrimeType, AActor* Witness)
{
	// Ignore if we are the criminal
	if (Criminal == GetOwner())
	{
		return;
	}

	// Ignore if we're already dead
	if (const AIronvaleNPCCharacter* NPC = GetOwnerNPC())
	{
		if (NPC->IsDead())
		{
			return;
		}
	}

	// Check if this NPC can actually perceive the crime
	if (!CanWitnessCrime(Criminal))
	{
		return;
	}

	WitnessCrime(Criminal, CrimeType);
}

// -----------------------------------------------------------------------------
// Archetype-specific response helpers
// -----------------------------------------------------------------------------

void UIronvaleCrimeResponseComponent::HandleGuardPursue(AActor* Criminal)
{
	AIronvaleNPCCharacter* NPC = GetOwnerNPC();
	if (!NPC)
	{
		return;
	}

	UIronvaleScheduleComponent* ScheduleComp = NPC->GetScheduleComponent();
	if (!ScheduleComp)
	{
		return;
	}

	// Inject a high-priority combat override so the guard drops whatever they're
	// doing and pursues the criminal. The behavior tree should read the
	// CrimeWitnessed blackboard key and the ThreatActor to drive the actual
	// pursuit/attack behavior.
	FIronvaleScheduleOverride PursueOverride;
	PursueOverride.OverrideID = TEXT("CrimeResponse_Pursue");
	PursueOverride.Activity = EIronvaleActivity::Combat;
	PursueOverride.Location = NAME_None; // Location is dynamic — driven by ThreatActor
	PursueOverride.Duration = 0.5f;      // Half a game-hour; BT refreshes as needed
	PursueOverride.Priority = 200;       // Very high — trumps everything

	ScheduleComp->InjectOverride(PursueOverride);

	UE_LOG(LogIronvale, Log, TEXT("[CrimeResponse] Guard %s pursuing criminal %s"),
		*NPC->GetName(), *Criminal->GetName());
}

void UIronvaleCrimeResponseComponent::HandleFlee()
{
	AIronvaleNPCCharacter* NPC = GetOwnerNPC();
	if (!NPC)
	{
		return;
	}

	UIronvaleScheduleComponent* ScheduleComp = NPC->GetScheduleComponent();
	if (!ScheduleComp)
	{
		return;
	}

	// Inject a flee override — sends the NPC to their home location
	FIronvaleScheduleOverride FleeOverride;
	FleeOverride.OverrideID = TEXT("CrimeResponse_Flee");
	FleeOverride.Activity = EIronvaleActivity::Flee;
	FleeOverride.Location = NPC->HomeLocationID; // Run home
	FleeOverride.Duration = 0.25f;               // Quarter game-hour
	FleeOverride.Priority = 150;                 // High — trumps normal schedule

	ScheduleComp->InjectOverride(FleeOverride);

	UE_LOG(LogIronvale, Log, TEXT("[CrimeResponse] %s fleeing to %s"),
		*NPC->GetName(), *NPC->HomeLocationID.ToString());
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

bool UIronvaleCrimeResponseComponent::CanWitnessCrime(AActor* Criminal) const
{
	if (!Criminal)
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Distance check
	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector CriminalLocation = Criminal->GetActorLocation();
	const float DistSq = FVector::DistSquared(OwnerLocation, CriminalLocation);

	if (DistSq > FMath::Square(WitnessRange))
	{
		return false;
	}

	// Optional line-of-sight check
	if (bRequireLineOfSight)
	{
		const UWorld* World = GetWorld();
		if (!World)
		{
			return false;
		}

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		QueryParams.AddIgnoredActor(Criminal);

		// Trace from NPC eye height to criminal center
		const FVector TraceStart = OwnerLocation + FVector(0.0f, 0.0f, 160.0f); // Approximate eye height
		const FVector TraceEnd = CriminalLocation + FVector(0.0f, 0.0f, 90.0f);  // Approximate center mass

		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams);

		// If we hit something between us and the criminal, LOS is blocked
		if (bHit)
		{
			return false;
		}
	}

	return true;
}

AIronvaleNPCCharacter* UIronvaleCrimeResponseComponent::GetOwnerNPC() const
{
	return Cast<AIronvaleNPCCharacter>(GetOwner());
}

EIronvaleCrimeResponse UIronvaleCrimeResponseComponent::DetermineResponse(
	EIronvaleNPCArchetype Archetype) const
{
	switch (Archetype)
	{
	case EIronvaleNPCArchetype::Guard:
		return EIronvaleCrimeResponse::Pursue;

	case EIronvaleNPCArchetype::Merchant:
		return EIronvaleCrimeResponse::Flee;

	case EIronvaleNPCArchetype::Villager:
		return EIronvaleCrimeResponse::Report;

	case EIronvaleNPCArchetype::Noble:
		return EIronvaleCrimeResponse::Report;

	case EIronvaleNPCArchetype::Priest:
		return EIronvaleCrimeResponse::Report;

	case EIronvaleNPCArchetype::Child:
		return EIronvaleCrimeResponse::Flee;

	case EIronvaleNPCArchetype::Bandit:
		return EIronvaleCrimeResponse::Ignore;

	case EIronvaleNPCArchetype::Traveler:
		return EIronvaleCrimeResponse::Flee;

	default:
		return EIronvaleCrimeResponse::None;
	}
}

void UIronvaleCrimeResponseComponent::HandleCrimeMemoryExpired()
{
	UE_LOG(LogIronvale, Verbose, TEXT("[CrimeResponse] %s crime memory expired after %.0fs"),
		*GetOwner()->GetName(), CrimeMemoryDuration);

	ForgetCrime();
}
