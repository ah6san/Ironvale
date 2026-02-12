// =============================================================================
// SIronvaleHUDWidget.h — Slate-based HUD overlay widget
// Project Ironvale
//
// Pure Slate widget that displays:
//   - Health bar (red)
//   - Stamina bar (green)
//   - Need indicators (hunger, thirst, fatigue, cleanliness)
//   - Game time display
// No UMG assets required — everything is constructed in C++.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class AIronvaleHUD;
class SProgressBar;
class STextBlock;

class SIronvaleHUDWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SIronvaleHUDWidget) {}
		SLATE_ARGUMENT(AIronvaleHUD*, OwningHUD)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Owning HUD for accessing player state */
	TWeakObjectPtr<AIronvaleHUD> OwningHUD;

	// Vitals bars
	TSharedPtr<SProgressBar> HealthBar;
	TSharedPtr<SProgressBar> StaminaBar;

	// Need indicators
	TSharedPtr<STextBlock> HungerText;
	TSharedPtr<STextBlock> ThirstText;
	TSharedPtr<STextBlock> FatigueText;
	TSharedPtr<STextBlock> CleanlinessText;

	// Game time
	TSharedPtr<STextBlock> TimeText;

	/** Update all displayed values from player state */
	void RefreshValues();

	/** Helper: create a labeled progress bar row */
	TSharedRef<SWidget> MakeBarRow(const FText& Label, TSharedPtr<SProgressBar>& OutBar, FLinearColor BarColor);

	/** Helper: create a need text row */
	TSharedRef<SWidget> MakeNeedRow(const FText& Label, TSharedPtr<STextBlock>& OutText);
};
