// =============================================================================
// IronvaleScheduleTypes.h — NPC schedule and override data types
// Project Ironvale
//
// Defines the schedule entry struct used to compose an NPC's daily routine,
// plus override structs for quest-injected behaviors that temporarily replace
// the NPC's normal schedule.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleScheduleTypes.generated.h"

// =============================================================================
// SCHEDULE ENTRY
// =============================================================================

/**
 * A single entry in an NPC's daily routine table.
 *
 * Each entry defines a time window, an activity to perform, and the location
 * where that activity takes place. Entries are evaluated top-down by the
 * ScheduleComponent: the first entry whose time range contains the current
 * game hour (and whose priority is highest among overlapping entries) wins.
 *
 * WeatherOverrideEntry allows a fallback if the current weather is "bad"
 * (Rain, Storm, Snow). For example, a farmer works outdoors in clear weather
 * but retreats to a tavern during rain.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleScheduleEntry
{
	GENERATED_BODY()

	/** Hour when this schedule block begins (0.0 – 24.0, inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule",
		meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartHour = 0.0f;

	/** Hour when this schedule block ends (0.0 – 24.0, exclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule",
		meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float EndHour = 0.0f;

	/** Activity the NPC performs during this block */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	EIronvaleActivity Activity = EIronvaleActivity::Work;

	/** Named location tag (resolved at runtime to a world waypoint or volume) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FName Location;

	/**
	 * Priority for resolving overlapping schedule entries.
	 * Higher values win. Default schedule entries use 0; injected overrides
	 * typically use 10+.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Priority = 0;

	/**
	 * Optional alternate entry used when weather is adverse (Rain, Storm, Snow).
	 * If this pointer is valid the ScheduleComponent will use the override's
	 * Activity and Location instead of this entry's.
	 *
	 * Stored as a TSharedPtr-equivalent using a nested struct to keep it
	 * USTRUCT-compatible (USTRUCT cannot contain TSharedPtr). If the Location
	 * is NAME_None the override is considered unset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule",
		meta = (DisplayName = "Bad-Weather Activity"))
	EIronvaleActivity WeatherOverrideActivity = EIronvaleActivity::Leisure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule",
		meta = (DisplayName = "Bad-Weather Location"))
	FName WeatherOverrideLocation;

	/** Returns true if a bad-weather fallback is configured */
	bool HasWeatherOverride() const
	{
		return !WeatherOverrideLocation.IsNone();
	}

	/**
	 * Check whether the given game hour falls within this entry's time window.
	 * Handles overnight ranges (e.g. StartHour=22 EndHour=6).
	 */
	bool ContainsHour(float GameHour) const
	{
		if (StartHour <= EndHour)
		{
			// Normal daytime range
			return GameHour >= StartHour && GameHour < EndHour;
		}
		// Overnight wrap (e.g. 22:00 – 06:00)
		return GameHour >= StartHour || GameHour < EndHour;
	}
};

// =============================================================================
// SCHEDULE OVERRIDE (Quest-injected)
// =============================================================================

/**
 * A temporary schedule override injected by the quest system.
 *
 * When active, overrides take precedence over the NPC's normal schedule table
 * based on their Priority. Multiple overrides can coexist; the highest-priority
 * one wins.
 *
 * Duration is measured in game-hours. Once the duration expires the
 * ScheduleComponent removes the override automatically.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleScheduleOverride
{
	GENERATED_BODY()

	/** Unique ID so the quest system can cancel or query this override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FName OverrideID;

	/** The activity the NPC should perform while this override is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	EIronvaleActivity Activity = EIronvaleActivity::Travel;

	/** Target location for the overridden behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FName Location;

	/**
	 * How long (in game-hours) this override remains active.
	 * A value <= 0 means infinite (must be cleared manually).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule",
		meta = (ClampMin = "-1.0"))
	float Duration = 1.0f;

	/**
	 * Priority for resolution against normal schedule entries and other overrides.
	 * Should be higher than any normal schedule entry it needs to beat (default
	 * schedule entries use priority 0).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	int32 Priority = 100;

	/** Game-time hour when this override was injected (set by ScheduleComponent) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Schedule")
	float InjectedAtGameHour = 0.0f;

	/** The day count when this override was injected */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Schedule")
	int32 InjectedOnDay = 0;

	/** Check if this override has expired given the current game time */
	bool IsExpired(float CurrentGameHour, int32 CurrentDay) const
	{
		// Infinite duration
		if (Duration <= 0.0f)
		{
			return false;
		}

		// Calculate total elapsed game-hours since injection
		const float ElapsedDays = static_cast<float>(CurrentDay - InjectedOnDay);
		float ElapsedHours = (ElapsedDays * 24.0f) + (CurrentGameHour - InjectedAtGameHour);

		// Handle same-day case where the clock hasn't wrapped
		if (ElapsedHours < 0.0f)
		{
			ElapsedHours += 24.0f;
		}

		return ElapsedHours >= Duration;
	}
};
