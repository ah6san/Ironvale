// =============================================================================
// IronvaleQuestObjective.h — Single objective tracker for the quest system
// Project Ironvale
//
// Lightweight UObject that tracks progress toward a single quest objective.
// Owned by the quest manager's stage runtime data. Provides clean API for
// incrementing progress, checking completion, and reporting percent.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IronvaleQuestObjective.generated.h"

/**
 * Tracks a single quest objective's progress at runtime.
 *
 * Created by the quest manager when a quest stage is initialized. One tracker
 * per objective in the stage. Destroyed when the quest advances to the next
 * stage or is completed/failed.
 */
UCLASS(BlueprintType)
class IRONVALE_API UIronvaleQuestObjectiveTracker : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Initialize the tracker with the required count and flags.
	 * Called by the quest manager when setting up stage trackers.
	 */
	void Initialize(int32 InRequiredCount, bool bInOptional = false, bool bInHidden = false);

	/**
	 * Add progress toward this objective.
	 * @param Delta  Amount to add (usually 1). Clamped to [0, RequiredCount].
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest|Objective")
	void IncrementProgress(int32 Delta = 1);

	/**
	 * Set progress to a specific value (used by save/load).
	 * Clamped to [0, RequiredCount].
	 */
	void SetProgress(int32 NewCount);

	/** Is this objective complete? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	bool IsComplete() const { return bCompleted; }

	/** Get progress as a 0-1 float */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	float GetProgressPercent() const;

	/** Get the current count */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	int32 GetCurrentCount() const { return CurrentCount; }

	/** Get the required count */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	int32 GetRequiredCount() const { return RequiredCount; }

	/** Is this an optional objective? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	bool IsOptional() const { return bOptional; }

	/** Is this objective hidden from the player? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest|Objective")
	bool IsHidden() const { return bHidden; }

	/** Mark this objective as revealed (no longer hidden) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest|Objective")
	void Reveal() { bHidden = false; }

protected:
	/** Current progress count */
	UPROPERTY(VisibleAnywhere, Category = "Ironvale|Quest|Objective")
	int32 CurrentCount = 0;

	/** Target count needed for completion */
	UPROPERTY(VisibleAnywhere, Category = "Ironvale|Quest|Objective")
	int32 RequiredCount = 1;

	/** Whether this objective has been fulfilled */
	UPROPERTY(VisibleAnywhere, Category = "Ironvale|Quest|Objective")
	bool bCompleted = false;

	/** Whether this objective is optional for stage progression */
	UPROPERTY(VisibleAnywhere, Category = "Ironvale|Quest|Objective")
	bool bOptional = false;

	/** Whether this objective is hidden from the player until explicitly revealed */
	UPROPERTY(VisibleAnywhere, Category = "Ironvale|Quest|Objective")
	bool bHidden = false;
};
