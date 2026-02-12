// =============================================================================
// IronvalePlayerController.h — Player controller for Project Ironvale
// Project Ironvale
//
// Responsibilities:
//   - Configures mouse/input settings for first-person gameplay
//   - Creates and manages the HUD
//   - Provides pause/unpause functionality
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IronvalePlayerController.generated.h"

UCLASS()
class IRONVALE_API AIronvalePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AIronvalePlayerController();

protected:
	virtual void BeginPlay() override;
};
