// =============================================================================
// IronvaleLockOnComponent.cpp — Lock-on targeting implementation
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleLockOnComponent.h"
#include "Ironvale.h"
#include "Characters/IronvaleCharacterBase.h"
#include "Combat/IronvaleHealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

UIronvaleLockOnComponent::UIronvaleLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIronvaleLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LockedTarget)
	{
		if (!IsTargetValid(LockedTarget))
		{
			Unlock();
		}
		else
		{
			UpdateCameraRotation(DeltaTime);
		}
	}
}

void UIronvaleLockOnComponent::ToggleLockOn()
{
	if (LockedTarget)
	{
		Unlock();
	}
	else
	{
		AActor* Target = FindBestTarget();
		if (Target)
		{
			LockOnTo(Target);
		}
	}
}

void UIronvaleLockOnComponent::LockOnTo(AActor* Target)
{
	if (!Target || Target == GetOwner()) return;

	LockedTarget = Target;
	OnLockOnTargetChanged.Broadcast(LockedTarget);

	UE_LOG(LogIronvale, Verbose, TEXT("Locked on to: %s"), *Target->GetName());
}

void UIronvaleLockOnComponent::Unlock()
{
	if (LockedTarget)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Lock-on released from: %s"), *LockedTarget->GetName());
		LockedTarget = nullptr;
		OnLockOnTargetChanged.Broadcast(nullptr);
	}
}

void UIronvaleLockOnComponent::SwitchTarget(bool bRight)
{
	if (!LockedTarget) return;

	// Find all valid targets, then pick the one most to the left/right of current target
	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<AActor*> ValidTargets;
	TArray<AActor*> FoundActors;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Owner->GetActorLocation(),
		MaxLockOnDistance, TArray<TEnumAsByte<EObjectTypeQuery>>(), AIronvaleCharacterBase::StaticClass(),
		TArray<AActor*>{Owner}, FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor != LockedTarget && IsTargetValid(Actor))
		{
			ValidTargets.Add(Actor);
		}
	}

	if (ValidTargets.Num() == 0) return;

	// Sort by angle relative to camera forward
	const FVector OwnerForward = Owner->GetActorForwardVector();
	const FVector OwnerLocation = Owner->GetActorLocation();

	AActor* BestTarget = nullptr;
	float BestAngle = bRight ? 360.0f : -360.0f;

	for (AActor* Target : ValidTargets)
	{
		const FVector ToTarget = (Target->GetActorLocation() - OwnerLocation).GetSafeNormal();
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OwnerForward, ToTarget)));
		const float CrossZ = FVector::CrossProduct(OwnerForward, ToTarget).Z;
		const float SignedAngle = CrossZ >= 0 ? Angle : -Angle;

		if (bRight && SignedAngle > 0 && SignedAngle < BestAngle)
		{
			BestAngle = SignedAngle;
			BestTarget = Target;
		}
		else if (!bRight && SignedAngle < 0 && SignedAngle > BestAngle)
		{
			BestAngle = SignedAngle;
			BestTarget = Target;
		}
	}

	if (BestTarget)
	{
		LockOnTo(BestTarget);
	}
}

float UIronvaleLockOnComponent::GetDistanceToTarget() const
{
	if (!LockedTarget || !GetOwner()) return 0.0f;
	return FVector::Dist(GetOwner()->GetActorLocation(), LockedTarget->GetActorLocation());
}

AActor* UIronvaleLockOnComponent::FindBestTarget() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector OwnerForward = Owner->GetActorForwardVector();

	TArray<AActor*> FoundActors;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), OwnerLocation,
		MaxLockOnDistance, TArray<TEnumAsByte<EObjectTypeQuery>>(), AIronvaleCharacterBase::StaticClass(),
		TArray<AActor*>{Owner}, FoundActors);

	AActor* BestTarget = nullptr;
	float BestScore = -1.0f;

	for (AActor* Actor : FoundActors)
	{
		if (!IsTargetValid(Actor)) continue;

		const FVector ToTarget = Actor->GetActorLocation() - OwnerLocation;
		const float Distance = ToTarget.Size();
		const FVector DirectionToTarget = ToTarget.GetSafeNormal();

		// Angle check
		const float DotProduct = FVector::DotProduct(OwnerForward, DirectionToTarget);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

		if (AngleDeg > LockOnSearchAngle) continue;

		// Score: prefer closest targets that are most centered in view
		const float DistScore = 1.0f - (Distance / MaxLockOnDistance);
		const float AngleScore = 1.0f - (AngleDeg / LockOnSearchAngle);
		const float Score = DistScore * 0.4f + AngleScore * 0.6f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

void UIronvaleLockOnComponent::UpdateCameraRotation(float DeltaTime)
{
	if (!LockedTarget || !GetOwner()) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	// Calculate desired rotation toward target
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = LockedTarget->GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, TargetLocation);

	// Smoothly interpolate camera rotation
	const FRotator CurrentRotation = PC->GetControlRotation();
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, LookAtRotation,
		DeltaTime, CameraRotationSpeed);

	PC->SetControlRotation(NewRotation);
}

bool UIronvaleLockOnComponent::IsTargetValid(AActor* Target) const
{
	if (!Target || !GetOwner()) return false;
	if (Target == GetOwner()) return false;

	// Check if target is alive
	if (UIronvaleHealthComponent* Health = Target->FindComponentByClass<UIronvaleHealthComponent>())
	{
		if (!Health->IsAlive()) return false;
	}

	// Check distance
	const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	if (Distance > AutoUnlockDistance) return false;

	return true;
}
