// =============================================================================
// IronvaleLockOnComponent.h — Target lock-on for combat
// Project Ironvale
//
// Scans for valid targets in front of the player, locks camera rotation
// toward the target, and provides target info to the combat system.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleLockOnComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChangedSignature, AActor*, NewTarget);

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleLockOnComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Toggle lock-on: if locked, unlock; if unlocked, find nearest valid target */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|LockOn")
	void ToggleLockOn();

	/** Force lock onto a specific target */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|LockOn")
	void LockOnTo(AActor* Target);

	/** Release current lock-on */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|LockOn")
	void Unlock();

	/** Switch to next target (while locked on) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|LockOn")
	void SwitchTarget(bool bRight = true);

	UFUNCTION(BlueprintPure, Category = "Ironvale|LockOn")
	bool IsLockedOn() const { return LockedTarget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|LockOn")
	AActor* GetLockedTarget() const { return LockedTarget; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|LockOn")
	float GetDistanceToTarget() const;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|LockOn")
	FOnLockOnTargetChangedSignature OnLockOnTargetChanged;

	// --- Config ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|LockOn")
	float MaxLockOnDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|LockOn")
	float LockOnSearchAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|LockOn")
	float CameraRotationSpeed = 10.0f;

	/** Auto-unlock if target moves beyond this distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|LockOn")
	float AutoUnlockDistance = 2000.0f;

protected:
	UPROPERTY()
	AActor* LockedTarget = nullptr;

	/** Find the best target in front of the player */
	AActor* FindBestTarget() const;

	/** Rotate camera smoothly toward locked target */
	void UpdateCameraRotation(float DeltaTime);

	/** Check if target is still valid (alive, in range, visible) */
	bool IsTargetValid(AActor* Target) const;
};
