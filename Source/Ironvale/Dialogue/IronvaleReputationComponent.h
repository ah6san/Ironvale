// =============================================================================
// IronvaleReputationComponent.h — Per-faction reputation subsystem
// Project Ironvale
//
// Game instance subsystem — persists across level loads.
// Stores reputation values for all factions and handles propagation to allies/enemies.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleReputationTypes.h"
#include "IronvaleReputationComponent.generated.h"

UCLASS()
class IRONVALE_API UIronvaleReputationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// =========================================================================
	// REPUTATION ACCESS
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	float GetReputation(FName FactionID) const;

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Reputation")
	void ModifyReputation(FName FactionID, float Delta, bool bPropagate = true);

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Reputation")
	void SetReputation(FName FactionID, float Value);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	bool IsFactionHostile(FName FactionID) const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	bool IsFactionFriendly(FName FactionID) const;

	/** Get all reputation entries (for save/load and UI) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	TArray<FIronvaleReputationEntry> GetAllReputations() const;

	/** Load reputations from save data */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Reputation")
	void LoadReputations(const TArray<FIronvaleReputationEntry>& SavedReps);

	/** Price multiplier for trading with a faction */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	float GetPriceMultiplier(FName FactionID) const;

	// =========================================================================
	// FACTION DATA
	// =========================================================================

	/** DataTable containing faction definitions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Reputation")
	UDataTable* FactionDataTable;

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Reputation")
	void LoadFactionData(UDataTable* InTable);

protected:
	/** Current reputation values per faction */
	TMap<FName, float> ReputationValues;

	/** Cached faction data */
	TMap<FName, FIronvaleFactionData> FactionData;

	/** Propagate reputation change to allied/enemy factions */
	void PropagateReputationChange(FName SourceFaction, float Delta);
};
