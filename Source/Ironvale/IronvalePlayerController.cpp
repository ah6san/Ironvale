// =============================================================================
// IronvalePlayerController.cpp — Player controller implementation
// Project Ironvale
// =============================================================================

#include "IronvalePlayerController.h"
#include "Ironvale.h"

AIronvalePlayerController::AIronvalePlayerController()
{
}

void AIronvalePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Configure input mode for first-person gameplay
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(false);

	UE_LOG(LogIronvale, Log, TEXT("IronvalePlayerController initialized"));
}
