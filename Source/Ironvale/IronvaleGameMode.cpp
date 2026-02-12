// =============================================================================
// IronvaleGameMode.cpp — Base game mode implementation
// Project Ironvale
// =============================================================================

#include "IronvaleGameMode.h"
#include "Ironvale.h"
#include "UObject/ConstructorHelpers.h"

AIronvaleGameMode::AIronvaleGameMode()
{
	// Default classes — override in Blueprints for final game
	// DefaultPawnClass will be set to BP_IronvalePlayerCharacter in editor
	// PlayerControllerClass will be set to BP_IronvalePlayerController in editor
}

void AIronvaleGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	UE_LOG(LogIronvale, Log, TEXT("Ironvale game initializing on map: %s"), *MapName);
}

void AIronvaleGameMode::StartPlay()
{
	Super::StartPlay();
	UE_LOG(LogIronvale, Log, TEXT("Ironvale game starting play"));
}

void AIronvaleGameMode::HandlePlayerDeath(AController* DeadPlayer)
{
	if (!DeadPlayer) return;

	UE_LOG(LogIronvale, Log, TEXT("Player died. Game over in %.1f seconds."), DeathToGameOverDelay);

	// Disable input on dead player
	if (APawn* Pawn = DeadPlayer->GetPawn())
	{
		Pawn->DisableInput(Cast<APlayerController>(DeadPlayer));
	}

	// Schedule game-over screen after delay
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, DeadPlayer]()
	{
		// In a full implementation: show game-over UI, offer load/restart
		UE_LOG(LogIronvale, Log, TEXT("Game over screen should appear now"));
	}, DeathToGameOverDelay, false);
}
