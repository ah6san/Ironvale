// =============================================================================
// IronvaleGameInstance.h — Persistent game instance (survives level transitions)
// Project Ironvale
//
// Responsibilities:
//   - Holds global game state that persists across level loads
//   - Manages world flags (set by quests, dialogue, scripts)
//   - Provides access to the save system
//   - Tracks total play time
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/IronvaleSaveTypes.h"
#include "IronvaleGameInstance.generated.h"

UCLASS()
class IRONVALE_API UIronvaleGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UIronvaleGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	// =========================================================================
	// WORLD FLAGS — Global bool flags set by quests/dialogue/scripts
	// =========================================================================

	/** Set a world flag (broadcasts to EventBus) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|WorldState")
	void SetWorldFlag(FName FlagName, bool bValue = true);

	/** Check if a world flag is set */
	UFUNCTION(BlueprintPure, Category = "Ironvale|WorldState")
	bool GetWorldFlag(FName FlagName) const;

	/** Get all world flags (for save/load) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|WorldState")
	const TMap<FName, bool>& GetAllWorldFlags() const { return WorldFlags; }

	/** Restore world flags from save data */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|WorldState")
	void LoadWorldFlags(const TMap<FName, bool>& SavedFlags);

	// =========================================================================
	// PLAY TIME TRACKING
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Meta")
	float GetTotalPlayTimeSeconds() const { return TotalPlayTimeSeconds; }

	// =========================================================================
	// CURRENT SAVE SLOT
	// =========================================================================

	/** Currently active save slot name (empty = no save loaded) */
	UPROPERTY(BlueprintReadWrite, Category = "Ironvale|Save")
	FString CurrentSaveSlot;

protected:
	UPROPERTY()
	TMap<FName, bool> WorldFlags;

	UPROPERTY()
	float TotalPlayTimeSeconds = 0.0f;

	FTimerHandle PlayTimeTimerHandle;

	void TickPlayTime();
};
