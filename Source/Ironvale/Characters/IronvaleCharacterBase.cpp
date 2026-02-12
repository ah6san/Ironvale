// =============================================================================
// IronvaleCharacterBase.cpp — Base character implementation
// Project Ironvale
// =============================================================================

#include "Characters/IronvaleCharacterBase.h"
#include "Ironvale.h"
#include "Combat/IronvaleCombatComponent.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Combat/IronvaleHealthComponent.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Inventory/IronvaleEquipmentComponent.h"
#include "Core/IronvaleEventBus.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AIronvaleCharacterBase::AIronvaleCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	CombatComponent = CreateDefaultSubobject<UIronvaleCombatComponent>(TEXT("CombatComponent"));
	StaminaComponent = CreateDefaultSubobject<UIronvaleStaminaComponent>(TEXT("StaminaComponent"));
	HealthComponent = CreateDefaultSubobject<UIronvaleHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UIronvaleInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<UIronvaleEquipmentComponent>(TEXT("EquipmentComponent"));
}

void AIronvaleCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Bind health depletion to death handler
	if (HealthComponent)
	{
		HealthComponent->OnHealthDepleted.AddDynamic(this, &AIronvaleCharacterBase::HandleHealthDepleted);
	}
}

void AIronvaleCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AIronvaleCharacterBase::Die(AActor* Killer)
{
	if (bIsDead) return;

	bIsDead = true;

	UE_LOG(LogIronvale, Log, TEXT("Character %s died. Killed by %s"),
		*CharacterID.ToString(),
		Killer ? *Killer->GetName() : TEXT("unknown"));

	// Transition combat state to Dead
	if (CombatComponent)
	{
		CombatComponent->ForceState(EIronvaleCombatState::Dead);
	}

	// Enable ragdoll for death physics
	EnableRagdoll();

	// Disable movement
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	// Disable collision with other characters (but keep world collision for ragdoll)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// Broadcast death event
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnCharacterDeath.Broadcast(this, Killer);
	}
}

void AIronvaleCharacterBase::HandleHealthDepleted(AActor* DamageCauser)
{
	Die(DamageCauser);
}

void AIronvaleCharacterBase::EnableRagdoll()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetAllBodiesSimulatePhysics(true);
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->bBlendPhysics = true;
	}

	// Disable capsule collision so ragdoll is the only collider
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AIronvaleCharacterBase::DisableRagdoll()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->bBlendPhysics = false;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}
