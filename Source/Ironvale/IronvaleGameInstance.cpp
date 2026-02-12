// =============================================================================
// IronvaleGameInstance.cpp — Persistent game instance implementation
// Project Ironvale
// =============================================================================

#include "IronvaleGameInstance.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "TimerManager.h"
#include "Engine/World.h"

UIronvaleGameInstance::UIronvaleGameInstance()
{
}

void UIronvaleGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogIronvale, Log, TEXT("Ironvale game instance initialized"));

	// Track play time every second
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PlayTimeTimerHandle, this,
			&UIronvaleGameInstance::TickPlayTime, 1.0f, true);
	}
}

void UIronvaleGameInstance::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayTimeTimerHandle);
	}

	Super::Shutdown();
	UE_LOG(LogIronvale, Log, TEXT("Ironvale game instance shutdown. Total play time: %.0f seconds"),
		TotalPlayTimeSeconds);
}

void UIronvaleGameInstance::SetWorldFlag(FName FlagName, bool bValue)
{
	WorldFlags.Add(FlagName, bValue);

	UE_LOG(LogIronvale, Verbose, TEXT("World flag set: %s = %s"),
		*FlagName.ToString(), bValue ? TEXT("true") : TEXT("false"));

	// Broadcast to event bus so other systems can react
	if (bValue)
	{
		if (UWorld* World = GetWorld())
		{
			if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
			{
				EventBus->OnWorldFlagSet.Broadcast(FlagName);
			}
		}
	}
}

bool UIronvaleGameInstance::GetWorldFlag(FName FlagName) const
{
	const bool* Found = WorldFlags.Find(FlagName);
	return Found ? *Found : false;
}

void UIronvaleGameInstance::LoadWorldFlags(const TMap<FName, bool>& SavedFlags)
{
	WorldFlags = SavedFlags;
	UE_LOG(LogIronvale, Log, TEXT("Loaded %d world flags from save"), WorldFlags.Num());
}

void UIronvaleGameInstance::TickPlayTime()
{
	TotalPlayTimeSeconds += 1.0f;
}
