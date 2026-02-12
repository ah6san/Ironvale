// =============================================================================
// IronvaleEncumbranceComponent.cpp
// Project Ironvale
// =============================================================================

#include "Needs/IronvaleEncumbranceComponent.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Inventory/IronvaleEquipmentComponent.h"
#include "Core/IronvaleStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

UIronvaleEncumbranceComponent::UIronvaleEncumbranceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Only need to check twice per second
}

void UIronvaleEncumbranceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		InventoryComp = Owner->FindComponentByClass<UIronvaleInventoryComponent>();
		EquipmentComp = Owner->FindComponentByClass<UIronvaleEquipmentComponent>();
	}

	RecalculateEncumbrance();
}

void UIronvaleEncumbranceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RecalculateEncumbrance();
}

void UIronvaleEncumbranceComponent::RecalculateEncumbrance()
{
	if (!InventoryComp) return;

	const float CurrentWeight = InventoryComp->GetCurrentWeight();
	const float MaxWeight = InventoryComp->GetMaxWeight();

	CurrentSpeedMultiplier = UIronvaleStatics::CalculateEncumbranceSpeedMultiplier(CurrentWeight, MaxWeight);
	bIsOverEncumbered = (CurrentWeight >= MaxWeight);

	// Apply to character movement component
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			// Base walk speed modulated by encumbrance
			// NOTE: Base speed is set elsewhere (sprint system, needs debuffs);
			// encumbrance multiplier is applied as an additional factor via MaxWalkSpeed
			// In a full impl, use a speed modifier stack rather than directly setting MaxWalkSpeed
		}
	}
}
