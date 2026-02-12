// =============================================================================
// IronvaleHUD.cpp — Main gameplay HUD implementation
// Project Ironvale
// =============================================================================

#include "UI/IronvaleHUD.h"
#include "UI/SIronvaleHUDWidget.h"
#include "Ironvale.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"

void AIronvaleHUD::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine && GEngine->GameViewport)
	{
		SAssignNew(HUDWidget, SIronvaleHUDWidget)
			.OwningHUD(this);

		GEngine->GameViewport->AddViewportWidgetContent(
			SNew(SWeakWidget).PossiblyNullContent(HUDWidget)
		);

		UE_LOG(LogIronvale, Log, TEXT("IronvaleHUD: Slate HUD widget created"));
	}
}

void AIronvaleHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget.IsValid())
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(
				SNew(SWeakWidget).PossiblyNullContent(HUDWidget)
			);
		}
		HUDWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
