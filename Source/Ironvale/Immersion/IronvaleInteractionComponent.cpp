// =============================================================================
// IronvaleInteractionComponent.cpp — Player-side interaction detection impl
// Project Ironvale
// =============================================================================

#include "Immersion/IronvaleInteractionComponent.h"
#include "Immersion/IronvaleInteractableComponent.h"
#include "Ironvale.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

UIronvaleInteractionComponent::UIronvaleInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Interaction trace does not need to run at full framerate — 30 Hz is sufficient
	PrimaryComponentTick.TickInterval = 0.033f;
}

void UIronvaleInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UIronvaleInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PerformInteractionTrace();
}

// =============================================================================
// TRACE LOGIC
// =============================================================================

void UIronvaleInteractionComponent::PerformInteractionTrace()
{
	FVector TraceStart;
	FVector TraceEnd;
	GetTracePoints(TraceStart, TraceEnd);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	bool bHit = false;

	if (TraceSphereRadius > 0.0f)
	{
		// Sphere sweep for easier targeting of small objects
		const FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceSphereRadius);
		bHit = GetWorld()->SweepSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			TraceChannel,
			SphereShape,
			QueryParams
		);
	}
	else
	{
		// Pure line trace
		bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			TraceChannel,
			QueryParams
		);
	}

	if (bHit && HitResult.GetActor())
	{
		// Look for an interactable component on the hit actor
		UIronvaleInteractableComponent* HitInteractable =
			HitResult.GetActor()->FindComponentByClass<UIronvaleInteractableComponent>();

		if (HitInteractable && HitInteractable->bIsEnabled)
		{
			SetCurrentTarget(HitInteractable);
			return;
		}
	}

	// Nothing valid found — clear target
	SetCurrentTarget(nullptr);
}

void UIronvaleInteractionComponent::GetTracePoints(FVector& OutStart, FVector& OutEnd) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		OutStart = GetOwner()->GetActorLocation();
		OutEnd = OutStart + GetOwner()->GetActorForwardVector() * InteractionRange;
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (PC && PC->PlayerCameraManager)
	{
		// Use the camera for first-person accuracy — trace goes from camera center outward
		OutStart = PC->PlayerCameraManager->GetCameraLocation();
		OutEnd = OutStart + PC->PlayerCameraManager->GetCameraRotation().Vector() * InteractionRange;
	}
	else
	{
		// Fallback for AI-controlled pawns or edge cases
		OutStart = OwnerPawn->GetPawnViewLocation();
		OutEnd = OutStart + OwnerPawn->GetViewRotation().Vector() * InteractionRange;
	}
}

void UIronvaleInteractionComponent::SetCurrentTarget(UIronvaleInteractableComponent* NewTarget)
{
	UIronvaleInteractableComponent* OldTarget = CurrentTarget.Get();

	if (OldTarget == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;
	OnInteractionTargetChanged.Broadcast(NewTarget);

	if (NewTarget)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Interaction target acquired: %s (%s)"),
			*NewTarget->GetOwner()->GetName(),
			*NewTarget->GetDisplayPrompt().ToString());
	}
	else
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Interaction target lost"));
	}
}

// =============================================================================
// INTERACTION EXECUTION
// =============================================================================

bool UIronvaleInteractionComponent::TryInteract()
{
	UIronvaleInteractableComponent* Target = CurrentTarget.Get();
	if (!Target)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("TryInteract: No current target"));
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	return Target->Interact(OwnerActor);
}

AActor* UIronvaleInteractionComponent::GetCurrentTargetActor() const
{
	const UIronvaleInteractableComponent* Target = CurrentTarget.Get();
	return Target ? Target->GetOwner() : nullptr;
}

bool UIronvaleInteractionComponent::HasValidTarget() const
{
	const UIronvaleInteractableComponent* Target = CurrentTarget.Get();
	return Target != nullptr && Target->bIsEnabled;
}
