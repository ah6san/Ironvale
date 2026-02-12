// =============================================================================
// IronvaleGameMode.cpp — Base game mode implementation
// Project Ironvale
// =============================================================================

#include "IronvaleGameMode.h"
#include "Ironvale.h"
#include "IronvalePlayerController.h"
#include "IronvaleGameState.h"
#include "Characters/IronvalePlayerCharacter.h"
#include "UI/IronvaleHUD.h"
#include "World/IronvaleTestLevelBuilder.h"
#include "UObject/ConstructorHelpers.h"

AIronvaleGameMode::AIronvaleGameMode()
{
	// Set all default classes so the game is playable from C++ alone —
	// no Blueprint setup required. Override in Blueprints for final game.
	DefaultPawnClass = AIronvalePlayerCharacter::StaticClass();
	PlayerControllerClass = AIronvalePlayerController::StaticClass();
	HUDClass = AIronvaleHUD::StaticClass();
	GameStateClass = AIronvaleGameState::StaticClass();
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

	// Spawn the test level builder to generate a playable environment
	// This ensures the game is playable even on an empty map
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AIronvaleTestLevelBuilder>(AIronvaleTestLevelBuilder::StaticClass(),
			FTransform::Identity, SpawnParams);
		UE_LOG(LogIronvale, Log, TEXT("Test level builder spawned"));
	}
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
