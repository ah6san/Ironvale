// =============================================================================
// IronvaleInteractionTypes.h — Data types for the world interaction system
// Project Ironvale: First-person grounded medieval RPG
//
// Defines the interaction data struct used by InteractableComponent and
// InteractionComponent to describe how players interact with world objects.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleInteractionTypes.generated.h"

/**
 * Describes a single interaction available on a world object.
 *
 * Placed inside UIronvaleInteractableComponent to configure what happens
 * when a player interacts: what prompt to show, how long it takes, whether
 * it requires an item, and what animation to play.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleInteractionData
{
	GENERATED_BODY()

	/** The category of interaction — determines system-level handling (sit, read, harvest, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EIronvaleInteractionType InteractionType = EIronvaleInteractionType::Examine;

	/** Localized prompt shown in the HUD when the player looks at this object (e.g., "Sit Down", "Read Book") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText DisplayPrompt = FText::GetEmpty();

	/** Duration in seconds. 0 = instant interaction, > 0 = hold-to-interact with progress bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InteractionDuration = 0.0f;

	/** Item ID required to interact. NAME_None = no requirement (e.g., a key for a locked door) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName RequiredItemID = NAME_None;

	/** Animation montage section to play on the interacting character. NAME_None = no animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName AnimationToPlay = NAME_None;

	/** If true, the player must be roughly facing the object to interact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bRequiresFacing = true;

	/** Returns true if this interaction has a non-zero duration (hold-to-interact) */
	bool IsTimedInteraction() const { return InteractionDuration > 0.0f; }

	/** Returns true if this interaction requires a specific item */
	bool RequiresItem() const { return RequiredItemID != NAME_None; }
};
