// =============================================================================
// IronvaleInteractableComponent.h — World-side interactable marker component
// Project Ironvale: First-person grounded medieval RPG
//
// Placed on any world object the player can interact with: chairs, forges,
// herbs, books, doors, chests, beds, etc. Describes what the interaction does
// and validates whether it can be performed.
//
// The player-side UIronvaleInteractionComponent detects these via line trace
// and calls CanInteract / Interact when the player presses the interact key.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleInteractionTypes.h"
#include "IronvaleInteractableComponent.generated.h"

class UIronvaleInventoryComponent;

// Delegate broadcast when an interaction completes (instant or after duration elapses)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleInteractionCompleteSignature,
	AActor*, Interactor, EIronvaleInteractionType, InteractionType);

/**
 * Component placed on world objects to make them interactable.
 *
 * Configurable via InteractionData — supports instant and timed interactions,
 * item requirements, facing checks, single-use objects, and animation playback.
 *
 * Cross-platform: uses only engine-native UActorComponent. No platform-specific code.
 */
UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleInteractableComponent();

	// =========================================================================
	// INTERACTION DATA
	// =========================================================================

	/** Configuration for this interactable — set in the editor per-instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction")
	FIronvaleInteractionData InteractionData;

	/** Whether this interactable is currently enabled and available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction")
	bool bIsEnabled = true;

	/** If true, this interactable can only be used once (e.g., herb picked, chest looted) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction")
	bool bSingleUse = false;

	/** Tracks whether a single-use interactable has been consumed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Interaction")
	bool bUsed = false;

	// =========================================================================
	// METHODS
	// =========================================================================

	/**
	 * Check if the given actor can interact with this object right now.
	 *
	 * Validates:
	 *   - Component is enabled and not already used (if single-use)
	 *   - Actor is valid and alive
	 *   - Required item is in the actor's inventory (if any)
	 *   - Actor is facing the object (if required)
	 *
	 * @param Interactor  The actor attempting to interact (typically the player character)
	 * @return true if all requirements are met
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Interaction")
	bool CanInteract(AActor* Interactor) const;

	/**
	 * Execute the interaction. Called by UIronvaleInteractionComponent after CanInteract passes.
	 *
	 * For instant interactions (Duration == 0): immediately completes and broadcasts delegate.
	 * For timed interactions (Duration > 0): starts a timer, broadcasts on completion.
	 *
	 * @param Interactor  The actor performing the interaction
	 * @return true if the interaction was started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Interaction")
	bool Interact(AActor* Interactor);

	/** Get the display prompt text for UI */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Interaction")
	FText GetDisplayPrompt() const { return InteractionData.DisplayPrompt; }

	/** Get the interaction type */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Interaction")
	EIronvaleInteractionType GetInteractionType() const { return InteractionData.InteractionType; }

	/** Programmatically enable/disable this interactable */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Interaction")
	void SetEnabled(bool bNewEnabled) { bIsEnabled = bNewEnabled; }

	/** Reset a single-use interactable so it can be used again */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Interaction")
	void ResetUsed() { bUsed = false; }

	// =========================================================================
	// DELEGATES
	// =========================================================================

	/** Broadcast when the interaction completes (after duration for timed, immediately for instant) */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Interaction")
	FOnIronvaleInteractionCompleteSignature OnInteractionComplete;

protected:
	/** Timer handle for timed interactions */
	FTimerHandle InteractionTimerHandle;

	/** Cached interactor for timed interaction completion callback */
	UPROPERTY()
	TWeakObjectPtr<AActor> PendingInteractor;

	/** Called when a timed interaction's duration elapses */
	void OnTimedInteractionComplete();

	/** Finalize the interaction: mark as used, broadcast delegate */
	void CompleteInteraction(AActor* Interactor);

	/** Check if the interactor has the required item in their inventory */
	bool HasRequiredItem(AActor* Interactor) const;

	/** Check if the interactor is roughly facing this object */
	bool IsFacingObject(AActor* Interactor) const;
};
