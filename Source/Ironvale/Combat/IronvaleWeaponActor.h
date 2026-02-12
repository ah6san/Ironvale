// =============================================================================
// IronvaleWeaponActor.h — Physical weapon actor with trace-based hit detection
// Project Ironvale
//
// Spawned and attached to character hand sockets when a weapon is equipped.
// During attack animations, anim notifies enable/disable the weapon trace
// which sweeps along the weapon to detect hits.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronvaleCombatTypes.h"
#include "Inventory/IronvaleItemData.h"
#include "IronvaleWeaponActor.generated.h"

UCLASS()
class IRONVALE_API AIronvaleWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AIronvaleWeaponActor();

	/** Initialize weapon from item data */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weapon")
	void InitFromItemData(const FIronvaleItemData& InItemData);

	// =========================================================================
	// TRACE CONTROL (called by anim notifies)
	// =========================================================================

	/** Start weapon trace — call at the start of attack animation hit window */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weapon")
	void EnableTrace();

	/** Stop weapon trace — call at the end of attack animation hit window */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weapon")
	void DisableTrace();

	UFUNCTION(BlueprintPure, Category = "Ironvale|Weapon")
	bool IsTracing() const { return bIsTracing; }

	// =========================================================================
	// DATA
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Weapon")
	const FIronvaleItemData& GetWeaponData() const { return WeaponData; }

	/** The owning character */
	UPROPERTY(BlueprintReadWrite, Category = "Ironvale|Weapon")
	TWeakObjectPtr<AActor> OwnerCharacter;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Weapon")
	UStaticMeshComponent* WeaponMeshComponent;

	/** Trace start point (near handle) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Weapon")
	USceneComponent* TraceStart;

	/** Trace end point (weapon tip) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Weapon")
	USceneComponent* TraceEnd;

	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Weapon")
	FIronvaleItemData WeaponData;

	/** Trace radius for sphere sweep */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Weapon")
	float TraceRadius = 5.0f;

	bool bIsTracing = false;

	/** Actors already hit during this trace window (prevent double-hits) */
	TArray<TWeakObjectPtr<AActor>> HitActorsThisSwing;

	/** Previous frame trace positions (for interpolated sweep) */
	FVector PreviousTraceStartPos = FVector::ZeroVector;
	FVector PreviousTraceEndPos = FVector::ZeroVector;

	/** Perform the weapon sweep trace */
	void PerformWeaponTrace();

	/** Process a trace hit result */
	void ProcessHit(const FHitResult& Hit);

	/**
	 * Determine which armor zone was hit based on the bone name.
	 * Maps skeletal bone names to EIronvaleArmorZone.
	 */
	static EIronvaleArmorZone BoneToArmorZone(FName BoneName);
};
