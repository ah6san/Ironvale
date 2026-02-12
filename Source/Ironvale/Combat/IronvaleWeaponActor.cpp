// =============================================================================
// IronvaleWeaponActor.cpp — Weapon trace-based hit detection implementation
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleWeaponActor.h"
#include "Ironvale.h"
#include "Combat/IronvaleCombatComponent.h"
#include "Characters/IronvaleCharacterBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AIronvaleWeaponActor::AIronvaleWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Weapon mesh
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetupAttachment(Root);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Trace points along the weapon
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(Root);
	TraceStart->SetRelativeLocation(FVector(0, 0, 10)); // Near handle

	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(Root);
	TraceEnd->SetRelativeLocation(FVector(0, 0, 100)); // Weapon tip
}

void AIronvaleWeaponActor::InitFromItemData(const FIronvaleItemData& InItemData)
{
	WeaponData = InItemData;

	// Set trace end location based on weapon reach
	if (TraceEnd)
	{
		TraceEnd->SetRelativeLocation(FVector(0, 0, InItemData.WeaponReach));
	}

	// Load and set mesh (async via soft pointer)
	if (!InItemData.WorldMesh.IsNull())
	{
		UStaticMesh* Mesh = InItemData.WorldMesh.LoadSynchronous();
		if (Mesh && WeaponMeshComponent)
		{
			WeaponMeshComponent->SetStaticMesh(Mesh);
		}
	}
}

void AIronvaleWeaponActor::EnableTrace()
{
	bIsTracing = true;
	HitActorsThisSwing.Empty();

	// Store initial positions for interpolated sweep
	if (TraceStart && TraceEnd)
	{
		PreviousTraceStartPos = TraceStart->GetComponentLocation();
		PreviousTraceEndPos = TraceEnd->GetComponentLocation();
	}

	UE_LOG(LogIronvale, Verbose, TEXT("Weapon trace enabled"));
}

void AIronvaleWeaponActor::DisableTrace()
{
	bIsTracing = false;
	HitActorsThisSwing.Empty();
}

void AIronvaleWeaponActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsTracing)
	{
		PerformWeaponTrace();
	}
}

void AIronvaleWeaponActor::PerformWeaponTrace()
{
	if (!TraceStart || !TraceEnd) return;

	const FVector CurrentStartPos = TraceStart->GetComponentLocation();
	const FVector CurrentEndPos = TraceEnd->GetComponentLocation();

	// Sphere sweep from trace start to trace end
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerCharacter.Get());
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;
	// Return face index disabled for cross-platform compatibility
	QueryParams.bReturnFaceIndex = false;

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		CurrentStartPos,
		CurrentEndPos,
		FQuat::Identity,
		ECC_GameTraceChannel1, // WeaponTrace channel (defined in DefaultEngine.ini)
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (!Hit.GetActor()) continue;

			// Skip already-hit actors (prevent multi-hit per swing)
			bool bAlreadyHit = false;
			for (const auto& PrevHit : HitActorsThisSwing)
			{
				if (PrevHit.Get() == Hit.GetActor())
				{
					bAlreadyHit = true;
					break;
				}
			}

			if (!bAlreadyHit)
			{
				HitActorsThisSwing.Add(Hit.GetActor());
				ProcessHit(Hit);
			}
		}
	}

	// Store for next frame's interpolation
	PreviousTraceStartPos = CurrentStartPos;
	PreviousTraceEndPos = CurrentEndPos;
}

void AIronvaleWeaponActor::ProcessHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	// Only process hits on characters with combat components
	UIronvaleCombatComponent* TargetCombat = HitActor->FindComponentByClass<UIronvaleCombatComponent>();
	if (!TargetCombat) return;

	// Build hit data
	FIronvaleHitData HitData;
	HitData.HitActor = HitActor;
	HitData.HitLocation = Hit.ImpactPoint;
	HitData.HitNormal = Hit.ImpactNormal;
	HitData.HitBoneName = Hit.BoneName;
	HitData.HitZone = BoneToArmorZone(Hit.BoneName);

	// Get attack data from owner's combat component
	if (OwnerCharacter.IsValid())
	{
		if (UIronvaleCombatComponent* OwnerCombat = OwnerCharacter->FindComponentByClass<UIronvaleCombatComponent>())
		{
			HitData.AttackData = OwnerCombat->GetCurrentAttackData();
		}
	}

	// Calculate impact velocity for physics response
	HitData.ImpactVelocity = (Hit.ImpactPoint - GetActorLocation()).GetSafeNormal() * 500.0f;

	// Process hit on target
	FIronvaleDamageResult Result = TargetCombat->ProcessIncomingHit(HitData);

	// Apply weapon durability damage to attacker's weapon
	if (OwnerCharacter.IsValid())
	{
		if (UIronvaleEquipmentComponent* OwnerEquip = OwnerCharacter->FindComponentByClass<UIronvaleEquipmentComponent>())
		{
			OwnerEquip->DamageMainHandWeapon(Result.WeaponDurabilityDamage);
		}
	}

	UE_LOG(LogIronvale, Verbose, TEXT("Weapon hit %s in zone %d for %.1f damage"),
		*HitActor->GetName(), static_cast<int32>(HitData.HitZone), Result.FinalDamage);
}

EIronvaleArmorZone AIronvaleWeaponActor::BoneToArmorZone(FName BoneName)
{
	const FString BoneStr = BoneName.ToString().ToLower();

	if (BoneStr.Contains(TEXT("head")) || BoneStr.Contains(TEXT("neck")))
		return EIronvaleArmorZone::Head;
	if (BoneStr.Contains(TEXT("spine")) || BoneStr.Contains(TEXT("chest")) || BoneStr.Contains(TEXT("pelvis")))
		return EIronvaleArmorZone::Torso;
	if (BoneStr.Contains(TEXT("l_upper")) || BoneStr.Contains(TEXT("l_forearm")) || BoneStr.Contains(TEXT("l_hand")) || BoneStr.Contains(TEXT("left")))
		return EIronvaleArmorZone::LeftArm;
	if (BoneStr.Contains(TEXT("r_upper")) || BoneStr.Contains(TEXT("r_forearm")) || BoneStr.Contains(TEXT("r_hand")) || BoneStr.Contains(TEXT("right")))
		return EIronvaleArmorZone::RightArm;
	if (BoneStr.Contains(TEXT("thigh")) || BoneStr.Contains(TEXT("calf")) || BoneStr.Contains(TEXT("leg")))
		return EIronvaleArmorZone::Legs;
	if (BoneStr.Contains(TEXT("foot")) || BoneStr.Contains(TEXT("toe")))
		return EIronvaleArmorZone::Feet;

	return EIronvaleArmorZone::Torso; // Default fallback
}
