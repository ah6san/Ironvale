// =============================================================================
// IronvaleGameMode.h — Base game mode for Project Ironvale
// Project Ironvale
//
// Responsibilities:
//   - Sets default pawn, controller, HUD, and game state classes
//   - Manages match/session lifecycle (start, end, respawn rules)
//   - Single-player focused but structured for potential multiplayer extension
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IronvaleGameMode.generated.h"

UCLASS()
class IRONVALE_API AIronvaleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIronvaleGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	/** Called when the player character dies — handles respawn or game over */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|GameMode")
	void HandlePlayerDeath(AController* DeadPlayer);

protected:
	/** Delay before showing game-over screen after death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|GameMode")
	float DeathToGameOverDelay = 3.0f;
};
