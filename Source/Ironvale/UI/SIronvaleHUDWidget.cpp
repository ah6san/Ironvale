// =============================================================================
// SIronvaleHUDWidget.cpp — Slate HUD widget implementation
// Project Ironvale
// =============================================================================

#include "UI/SIronvaleHUDWidget.h"
#include "UI/IronvaleHUD.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "Characters/IronvalePlayerCharacter.h"
#include "Combat/IronvaleHealthComponent.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Needs/IronvaleNeedsComponent.h"
#include "Core/IronvaleStatics.h"
#include "Core/IronvaleTypes.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

void SIronvaleHUDWidget::Construct(const FArguments& InArgs)
{
	OwningHUD = InArgs._OwningHUD;

	ChildSlot
	[
		SNew(SOverlay)

		// Bottom-left: Vitals (health + stamina bars)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(20.0f, 0.0f, 0.0f, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(250.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					MakeBarRow(NSLOCTEXT("IronvaleHUD", "Health", "HP"), HealthBar, FLinearColor(0.8f, 0.1f, 0.1f))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					MakeBarRow(NSLOCTEXT("IronvaleHUD", "Stamina", "ST"), StaminaBar, FLinearColor(0.2f, 0.7f, 0.2f))
				]
			]
		]

		// Bottom-right: Needs indicators
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 20.0f, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(160.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f)
				[
					MakeNeedRow(NSLOCTEXT("IronvaleHUD", "Hunger", "Hunger"), HungerText)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f)
				[
					MakeNeedRow(NSLOCTEXT("IronvaleHUD", "Thirst", "Thirst"), ThirstText)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f)
				[
					MakeNeedRow(NSLOCTEXT("IronvaleHUD", "Fatigue", "Fatigue"), FatigueText)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f)
				[
					MakeNeedRow(NSLOCTEXT("IronvaleHUD", "Cleanliness", "Clean"), CleanlinessText)
				]
			]
		]

		// Top-right: Game time
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 15.0f, 20.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
			.Padding(FMargin(8.0f, 4.0f))
			[
				SAssignNew(TimeText, STextBlock)
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.Text(FText::FromString(TEXT("08:00 - Day 1")))
			]
		]
	];
}

void SIronvaleHUDWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	RefreshValues();
}

void SIronvaleHUDWidget::RefreshValues()
{
	if (!OwningHUD.IsValid()) return;

	APlayerController* PC = OwningHUD->GetOwningPlayerController();
	if (!PC) return;

	AIronvalePlayerCharacter* PlayerChar = Cast<AIronvalePlayerCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	// Health
	if (HealthBar.IsValid())
	{
		if (UIronvaleHealthComponent* Health = PlayerChar->GetHealthComponent())
		{
			HealthBar->SetPercent(Health->GetHealthPercent());
		}
	}

	// Stamina
	if (StaminaBar.IsValid())
	{
		if (UIronvaleStaminaComponent* Stamina = PlayerChar->GetStaminaComponent())
		{
			StaminaBar->SetPercent(Stamina->GetStaminaPercent());
		}
	}

	// Needs
	if (UIronvaleNeedsComponent* Needs = PlayerChar->GetNeedsComponent())
	{
		auto FormatNeed = [Needs](EIronvaleNeedType Type) -> FString
		{
			const float Val = Needs->GetNeedValue(Type);
			return FString::Printf(TEXT("%.0f%%"), Val);
		};

		if (HungerText.IsValid())
			HungerText->SetText(FText::FromString(FormatNeed(EIronvaleNeedType::Hunger)));
		if (ThirstText.IsValid())
			ThirstText->SetText(FText::FromString(FormatNeed(EIronvaleNeedType::Thirst)));
		if (FatigueText.IsValid())
			FatigueText->SetText(FText::FromString(FormatNeed(EIronvaleNeedType::Fatigue)));
		if (CleanlinessText.IsValid())
			CleanlinessText->SetText(FText::FromString(FormatNeed(EIronvaleNeedType::Cleanliness)));
	}

	// Game time
	if (TimeText.IsValid())
	{
		if (UWorld* World = OwningHUD->GetWorld())
		{
			if (AIronvaleGameState* GS = Cast<AIronvaleGameState>(World->GetGameState()))
			{
				FString TimeStr = FString::Printf(TEXT("%s - Day %d"),
					*UIronvaleStatics::GameHourToTimeString(GS->GetGameTimeHours()),
					GS->GetDayCount());
				TimeText->SetText(FText::FromString(TimeStr));
			}
		}
	}
}

TSharedRef<SWidget> SIronvaleHUDWidget::MakeBarRow(const FText& Label, TSharedPtr<SProgressBar>& OutBar, FLinearColor BarColor)
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(30.0f)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.Text(Label)
			]
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.HeightOverride(14.0f)
			[
				SAssignNew(OutBar, SProgressBar)
				.Percent(1.0f)
				.FillColorAndOpacity(BarColor)
				.BackgroundImage(new FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f)))
			]
		];
}

TSharedRef<SWidget> SIronvaleHUDWidget::MakeNeedRow(const FText& Label, TSharedPtr<STextBlock>& OutText)
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
			.Text(Label)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(OutText, STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor::White))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.Text(FText::FromString(TEXT("100%")))
		];
}
