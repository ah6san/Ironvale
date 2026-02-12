// =============================================================================
// IronvaleScheduleComponent.cpp — NPC daily routine manager implementation
// Project Ironvale
// =============================================================================

#include "AI/IronvaleScheduleComponent.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "Core/IronvaleEventBus.h"
#include "GameFramework/GameStateBase.h"

UIronvaleScheduleComponent::UIronvaleScheduleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Only tick once per second — schedule changes are hour-granular
	PrimaryComponentTick.TickInterval = 1.0f;
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void UIronvaleScheduleComponent::BeginPlay()
{
	Super::BeginPlay();

	// Subscribe to the EventBus hour-changed event
	if (UWorld* World = GetWorld())
	{
		if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnHourChanged.AddDynamic(this, &UIronvaleScheduleComponent::HandleHourChanged);
		}
	}

	// Initial evaluation so the NPC starts with a valid activity
	EvaluateSchedule();

	UE_LOG(LogIronvale, Verbose, TEXT("[ScheduleComponent] %s initialized with %d schedule entries, %d overrides"),
		*GetOwner()->GetName(), DailySchedule.Num(), ActiveOverrides.Num());
}

void UIronvaleScheduleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from EventBus
	if (UWorld* World = GetWorld())
	{
		if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnHourChanged.RemoveDynamic(this, &UIronvaleScheduleComponent::HandleHourChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UIronvaleScheduleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Periodic override expiration check (hour-changed handles the main
	// re-evaluation, but this catches overrides that expire mid-hour)
	PurgeExpiredOverrides();
}

// -----------------------------------------------------------------------------
// Runtime queries
// -----------------------------------------------------------------------------

bool UIronvaleScheduleComponent::GetCurrentScheduleEntry(FIronvaleScheduleEntry& OutEntry) const
{
	const AIronvaleGameState* GS = GetIronvaleGameState();
	if (!GS)
	{
		return false;
	}

	const float CurrentHour = GS->GetGameTimeHours();
	const int32 CurrentDay = GS->GetDayCount();
	const bool bBadWeather = IsWeatherAdverse();

	// --- Phase 1: Check active overrides (highest priority first) ---
	int32 BestOverridePriority = TNumericLimits<int32>::Min();
	const FIronvaleScheduleOverride* WinningOverride = nullptr;

	for (const FIronvaleScheduleOverride& Override : ActiveOverrides)
	{
		if (Override.IsExpired(CurrentHour, CurrentDay))
		{
			continue;
		}
		if (Override.Priority > BestOverridePriority)
		{
			BestOverridePriority = Override.Priority;
			WinningOverride = &Override;
		}
	}

	// --- Phase 2: Check normal schedule entries ---
	int32 BestSchedulePriority = TNumericLimits<int32>::Min();
	const FIronvaleScheduleEntry* WinningEntry = nullptr;

	for (const FIronvaleScheduleEntry& Entry : DailySchedule)
	{
		if (Entry.ContainsHour(CurrentHour) && Entry.Priority > BestSchedulePriority)
		{
			BestSchedulePriority = Entry.Priority;
			WinningEntry = &Entry;
		}
	}

	// --- Phase 3: Resolve winner ---
	if (WinningOverride && WinningOverride->Priority >= BestSchedulePriority)
	{
		// Override wins — synthesize a schedule entry from it
		OutEntry.StartHour = 0.0f;
		OutEntry.EndHour = 24.0f;
		OutEntry.Activity = WinningOverride->Activity;
		OutEntry.Location = WinningOverride->Location;
		OutEntry.Priority = WinningOverride->Priority;
		return true;
	}

	if (WinningEntry)
	{
		// Normal schedule entry wins — apply weather override if needed
		OutEntry = *WinningEntry;
		if (bBadWeather && WinningEntry->HasWeatherOverride())
		{
			OutEntry.Activity = WinningEntry->WeatherOverrideActivity;
			OutEntry.Location = WinningEntry->WeatherOverrideLocation;
		}
		return true;
	}

	return false;
}

EIronvaleActivity UIronvaleScheduleComponent::GetCurrentActivity() const
{
	FIronvaleScheduleEntry Entry;
	if (GetCurrentScheduleEntry(Entry))
	{
		return Entry.Activity;
	}
	// Default fallback: idle / sleep
	return EIronvaleActivity::Sleep;
}

FName UIronvaleScheduleComponent::GetCurrentTargetLocation() const
{
	FIronvaleScheduleEntry Entry;
	if (GetCurrentScheduleEntry(Entry))
	{
		return Entry.Location;
	}
	return NAME_None;
}

// -----------------------------------------------------------------------------
// Override management
// -----------------------------------------------------------------------------

void UIronvaleScheduleComponent::InjectOverride(FIronvaleScheduleOverride Override)
{
	// Stamp injection time
	if (const AIronvaleGameState* GS = GetIronvaleGameState())
	{
		Override.InjectedAtGameHour = GS->GetGameTimeHours();
		Override.InjectedOnDay = GS->GetDayCount();
	}

	// Replace existing override with the same ID
	ClearOverride(Override.OverrideID);

	ActiveOverrides.Add(MoveTemp(Override));

	UE_LOG(LogIronvale, Log, TEXT("[ScheduleComponent] %s: Override '%s' injected (Activity=%d, Location=%s, Duration=%.1fh, Priority=%d)"),
		*GetOwner()->GetName(),
		*Override.OverrideID.ToString(),
		static_cast<int32>(Override.Activity),
		*Override.Location.ToString(),
		Override.Duration,
		Override.Priority);

	// Re-evaluate immediately — the new override may take effect now
	EvaluateSchedule();
}

void UIronvaleScheduleComponent::ClearOverride(FName OverrideID)
{
	const int32 Removed = ActiveOverrides.RemoveAll(
		[&OverrideID](const FIronvaleScheduleOverride& O) { return O.OverrideID == OverrideID; });

	if (Removed > 0)
	{
		UE_LOG(LogIronvale, Log, TEXT("[ScheduleComponent] %s: Override '%s' cleared"),
			*GetOwner()->GetName(), *OverrideID.ToString());
		EvaluateSchedule();
	}
}

void UIronvaleScheduleComponent::ClearAllOverrides()
{
	if (ActiveOverrides.Num() > 0)
	{
		ActiveOverrides.Empty();
		UE_LOG(LogIronvale, Log, TEXT("[ScheduleComponent] %s: All overrides cleared"),
			*GetOwner()->GetName());
		EvaluateSchedule();
	}
}

bool UIronvaleScheduleComponent::HasActiveOverride() const
{
	if (ActiveOverrides.Num() == 0)
	{
		return false;
	}

	const AIronvaleGameState* GS = GetIronvaleGameState();
	if (!GS)
	{
		return false;
	}

	const float CurrentHour = GS->GetGameTimeHours();
	const int32 CurrentDay = GS->GetDayCount();

	for (const FIronvaleScheduleOverride& Override : ActiveOverrides)
	{
		if (!Override.IsExpired(CurrentHour, CurrentDay))
		{
			return true;
		}
	}

	return false;
}

bool UIronvaleScheduleComponent::HasOverrideWithID(FName OverrideID) const
{
	const AIronvaleGameState* GS = GetIronvaleGameState();
	if (!GS)
	{
		return false;
	}

	const float CurrentHour = GS->GetGameTimeHours();
	const int32 CurrentDay = GS->GetDayCount();

	for (const FIronvaleScheduleOverride& Override : ActiveOverrides)
	{
		if (Override.OverrideID == OverrideID && !Override.IsExpired(CurrentHour, CurrentDay))
		{
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Event handlers
// -----------------------------------------------------------------------------

void UIronvaleScheduleComponent::HandleHourChanged(int32 NewHour)
{
	PurgeExpiredOverrides();
	EvaluateSchedule();
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void UIronvaleScheduleComponent::EvaluateSchedule()
{
	const EIronvaleActivity NewActivity = GetCurrentActivity();
	const FName NewLocation = GetCurrentTargetLocation();

	if (NewActivity != CachedActivity || NewLocation != CachedLocation)
	{
		CachedActivity = NewActivity;
		CachedLocation = NewLocation;

		OnScheduleEntryChanged.Broadcast(NewActivity, NewLocation);

		UE_LOG(LogIronvale, Verbose, TEXT("[ScheduleComponent] %s: Schedule changed → Activity=%d, Location=%s"),
			*GetOwner()->GetName(),
			static_cast<int32>(NewActivity),
			*NewLocation.ToString());
	}
}

void UIronvaleScheduleComponent::PurgeExpiredOverrides()
{
	const AIronvaleGameState* GS = GetIronvaleGameState();
	if (!GS)
	{
		return;
	}

	const float CurrentHour = GS->GetGameTimeHours();
	const int32 CurrentDay = GS->GetDayCount();

	const int32 Removed = ActiveOverrides.RemoveAll(
		[CurrentHour, CurrentDay](const FIronvaleScheduleOverride& O)
		{
			return O.IsExpired(CurrentHour, CurrentDay);
		});

	if (Removed > 0)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("[ScheduleComponent] %s: Purged %d expired override(s)"),
			*GetOwner()->GetName(), Removed);
	}
}

AIronvaleGameState* UIronvaleScheduleComponent::GetIronvaleGameState() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<AIronvaleGameState>(World->GetGameState());
	}
	return nullptr;
}

bool UIronvaleScheduleComponent::IsWeatherAdverse() const
{
	const AIronvaleGameState* GS = GetIronvaleGameState();
	if (!GS)
	{
		return false;
	}

	const EIronvaleWeatherState Weather = GS->GetCurrentWeather();
	return Weather == EIronvaleWeatherState::Rain
		|| Weather == EIronvaleWeatherState::Storm
		|| Weather == EIronvaleWeatherState::Snow;
}
