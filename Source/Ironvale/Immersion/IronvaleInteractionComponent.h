// =============================================================================
// IronvaleInteractionComponent.h — Player-side interaction detection and execution
// Project Ironvale: First-person grounded medieval RPG
//
// Attached to the player character. Each tick, performs a line trace from the
// camera to detect UIronvaleInteractableComponent on world objects. Exposes
// the current target for UI prompts and executes interactions on player input.
//
// Cross-platform: uses engine-native collision queries only.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleInteractionComponent.generated.h"

class UIronvaleInteractableComponent;

// Broadcast when the interactable the player is looking at changes (or becomes null)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleInteractionTargetChangedSignature,
	UIronvaleInteractableComponent*, NewTarget);

/**
 * Player-side interaction component.
 *
 * Performs a line trace from the camera each tick to find
 * UIronvaleInteractableComponent on world actors within InteractionRange.
 * Broadcasts OnInteractionTargetChanged when the target changes, so the
 * HUD can display the appropriate prompt.
 *
 * The player calls TryInteract() (bound to the interact input action) to
 * execute the interaction on the current target.
 */
UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleInteractionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Maximum distance (in cm) the player can interact with objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float InteractionRange = 300.0f;

	/** Collision channel used for the interaction line trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Radius for the sphere trace (0 = pure line trace, > 0 = sphere sweep for easier targeting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TraceSphereRadius = 10.0f;

	// =========================================================================
	// METHODS
	// =========================================================================

	/**
	 * Attempt to interact with the current target.
	 * Called when the player presses the interact key.
	 *
	 * @return true if an interaction was started
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Interaction")
	bool TryInteract();

	/**
	 * Get the interactable component the player is currently looking at.
	 * Returns nullptr if nothing is in range or the target is not interactable.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Interaction")
	UIronvaleInteractableComponent* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/**
	 * Get the actor that owns the current interaction target.
	 * Convenience method for UI display.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Interaction")
	AActor* GetCurrentTargetActor() const;

	/** Returns true if the player is currently looking at an interactable */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Interaction")
	bool HasValidTarget() const;

	// =========================================================================
	// DELEGATES
	// =========================================================================

	/** Broadcast when the interactable the player is looking at changes (including to null) */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Interaction")
	FOnIronvaleInteractionTargetChangedSignature OnInteractionTargetChanged;

protected:
	/** The interactable component the player is currently looking at */
	UPROPERTY()
	TWeakObjectPtr<UIronvaleInteractableComponent> CurrentTarget;

	/**
	 * Perform the line/sphere trace from the camera and update CurrentTarget.
	 * Called every tick.
	 */
	void PerformInteractionTrace();

	/**
	 * Calculate the trace start and end points from the player's camera.
	 * Uses the player controller's camera manager for first-person accuracy.
	 */
	void GetTracePoints(FVector& OutStart, FVector& OutEnd) const;

	/**
	 * Update the current target and broadcast if it changed.
	 * Handles both acquiring a new target and losing the old one.
	 */
	void SetCurrentTarget(UIronvaleInteractableComponent* NewTarget);
};
