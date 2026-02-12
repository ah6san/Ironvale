// =============================================================================
// IronvaleInteractableComponent.cpp — World-side interactable component impl
// Project Ironvale
// =============================================================================

#include "Immersion/IronvaleInteractableComponent.h"
#include "Ironvale.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Combat/IronvaleHealthComponent.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UIronvaleInteractableComponent::UIronvaleInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// =============================================================================
// VALIDATION
// =============================================================================

bool UIronvaleInteractableComponent::CanInteract(AActor* Interactor) const
{
	// Basic availability checks
	if (!bIsEnabled)
	{
		return false;
	}

	if (bSingleUse && bUsed)
	{
		return false;
	}

	if (!IsValid(Interactor))
	{
		return false;
	}

	// Check that interactor is alive (if they have a health component)
	if (const UIronvaleHealthComponent* HealthComp = Interactor->FindComponentByClass<UIronvaleHealthComponent>())
	{
		if (!HealthComp->IsAlive())
		{
			return false;
		}
	}

	// Check required item
	if (InteractionData.RequiresItem() && !HasRequiredItem(Interactor))
	{
		return false;
	}

	// Check facing requirement
	if (InteractionData.bRequiresFacing && !IsFacingObject(Interactor))
	{
		return false;
	}

	return true;
}

bool UIronvaleInteractableComponent::HasRequiredItem(AActor* Interactor) const
{
	if (!InteractionData.RequiresItem())
	{
		return true;
	}

	const UIronvaleInventoryComponent* InventoryComp = Interactor->FindComponentByClass<UIronvaleInventoryComponent>();
	if (!InventoryComp)
	{
		return false;
	}

	return InventoryComp->HasItem(InteractionData.RequiredItemID, 1);
}

bool UIronvaleInteractableComponent::IsFacingObject(AActor* Interactor) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Interactor)
	{
		return false;
	}

	// Calculate direction from interactor to this object
	const FVector ToObject = (Owner->GetActorLocation() - Interactor->GetActorLocation()).GetSafeNormal();
	const FVector ForwardDir = Interactor->GetActorForwardVector();

	// Dot product: 1.0 = perfectly facing, 0.0 = perpendicular, -1.0 = facing away
	// Require at least ~60 degree cone (dot > 0.5)
	const float DotProduct = FVector::DotProduct(ForwardDir, ToObject);

	return DotProduct > 0.5f;
}

// =============================================================================
// INTERACTION EXECUTION
// =============================================================================

bool UIronvaleInteractableComponent::Interact(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Interaction rejected on %s by %s — requirements not met"),
			*GetOwner()->GetName(), *Interactor->GetName());
		return false;
	}

	UE_LOG(LogIronvale, Log, TEXT("%s interacting with %s (Type: %d, Duration: %.1fs)"),
		*Interactor->GetName(),
		*GetOwner()->GetName(),
		static_cast<int32>(InteractionData.InteractionType),
		InteractionData.InteractionDuration);

	if (InteractionData.IsTimedInteraction())
	{
		// Start timed interaction — complete after duration elapses
		PendingInteractor = Interactor;

		GetWorld()->GetTimerManager().SetTimer(
			InteractionTimerHandle,
			this,
			&UIronvaleInteractableComponent::OnTimedInteractionComplete,
			InteractionData.InteractionDuration,
			false // Not looping
		);

		return true;
	}

	// Instant interaction — complete immediately
	CompleteInteraction(Interactor);
	return true;
}

void UIronvaleInteractableComponent::OnTimedInteractionComplete()
{
	AActor* Interactor = PendingInteractor.Get();
	if (!Interactor)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Timed interaction on %s completed but interactor is no longer valid"),
			*GetOwner()->GetName());
		return;
	}

	// Re-validate: the interactor may have moved, died, or lost the required item
	// during the interaction duration. We skip the facing check since they were
	// already committed to the interaction.
	if (!bIsEnabled || (bSingleUse && bUsed) || !IsValid(Interactor))
	{
		UE_LOG(LogIronvale, Log, TEXT("Timed interaction on %s cancelled — state changed during interaction"),
			*GetOwner()->GetName());
		PendingInteractor.Reset();
		return;
	}

	CompleteInteraction(Interactor);
	PendingInteractor.Reset();
}

void UIronvaleInteractableComponent::CompleteInteraction(AActor* Interactor)
{
	// Mark as used for single-use interactables
	if (bSingleUse)
	{
		bUsed = true;
	}

	// Broadcast completion delegate — listeners handle type-specific logic
	// (e.g., sitting state, reading UI, harvest loot generation)
	OnInteractionComplete.Broadcast(Interactor, InteractionData.InteractionType);

	UE_LOG(LogIronvale, Log, TEXT("Interaction complete on %s by %s (Type: %d)"),
		*GetOwner()->GetName(),
		*Interactor->GetName(),
		static_cast<int32>(InteractionData.InteractionType));
}
