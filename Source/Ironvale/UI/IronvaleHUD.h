// =============================================================================
// IronvaleHUD.h — Main gameplay HUD using Slate
// Project Ironvale
//
// Displays player vitals (health, stamina) and needs status using pure Slate
// widgets — no UMG/Blueprint assets required. This allows the game to run
// with zero editor-created UI assets.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IronvaleHUD.generated.h"

class SIronvaleHUDWidget;

UCLASS()
class IRONVALE_API AIronvaleHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	TSharedPtr<SIronvaleHUDWidget> HUDWidget;
};
