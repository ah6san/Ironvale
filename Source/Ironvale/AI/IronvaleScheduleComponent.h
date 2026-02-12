// =============================================================================
// IronvaleScheduleComponent.h — NPC daily routine manager
// Project Ironvale
//
// Attached to every NPC. Holds the NPC's daily schedule table and evaluates
// which activity + location is current, accounting for weather and quest
// overrides. The AI controller reads from this component to populate the
// behavior tree blackboard.
//
// Listens to EventBus::OnHourChanged to re-evaluate the active entry each
// game-hour.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/IronvaleScheduleTypes.h"
#include "IronvaleScheduleComponent.generated.h"

class UIronvaleEventBus;
class AIronvaleGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleScheduleEntryChangedSignature,
	EIronvaleActivity, NewActivity, FName, NewLocation);

/**
 * UIronvaleScheduleComponent
 *
 * Manages an NPC's daily routine. The schedule is a designer-authored list of
 * FIronvaleScheduleEntry rows evaluated against the game clock. Quest systems
 * can inject temporary overrides that supersede the normal schedule.
 *
 * The component does NOT drive movement or animation directly; it merely
 * exposes GetCurrentActivity() / GetCurrentTargetLocation() for the AI
 * controller to push into the blackboard.
 */
UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleScheduleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleScheduleComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// SCHEDULE TABLE (designer-authored)
	// =========================================================================

	/** The NPC's daily routine. Order does not matter — evaluated by time + priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Schedule")
	TArray<FIronvaleScheduleEntry> DailySchedule;

	// =========================================================================
	// RUNTIME QUERIES
	// =========================================================================

	/**
	 * Get the schedule entry that is active right now based on game time,
	 * weather, and any active overrides.
	 *
	 * @param OutEntry  Populated with the winning entry if one is found.
	 * @return True if a valid entry was resolved, false if the NPC has nothing
	 *         scheduled (will idle at home).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Schedule")
	bool GetCurrentScheduleEntry(FIronvaleScheduleEntry& OutEntry) const;

	/** Convenience — returns just the current activity enum */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Schedule")
	EIronvaleActivity GetCurrentActivity() const;

	/** Convenience — returns just the current target location name */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Schedule")
	FName GetCurrentTargetLocation() const;

	// =========================================================================
	// OVERRIDE MANAGEMENT (quest system interface)
	// =========================================================================

	/**
	 * Inject a temporary override into this NPC's schedule.
	 * If an override with the same OverrideID already exists, it is replaced.
	 *
	 * @param Override  The override definition. InjectedAtGameHour and
	 *                  InjectedOnDay will be stamped automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Schedule")
	void InjectOverride(FIronvaleScheduleOverride Override);

	/** Remove a specific override by its ID */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Schedule")
	void ClearOverride(FName OverrideID);

	/** Remove all active overrides */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Schedule")
	void ClearAllOverrides();

	/** Check whether any override is currently active (not expired) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Schedule")
	bool HasActiveOverride() const;

	/** Check whether a specific override is active */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Schedule")
	bool HasOverrideWithID(FName OverrideID) const;

	// =========================================================================
	// EVENTS
	// =========================================================================

	/** Fired when the resolved schedule entry changes (new activity or location) */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Schedule")
	FOnIronvaleScheduleEntryChangedSignature OnScheduleEntryChanged;

protected:
	// =========================================================================
	// INTERNAL STATE
	// =========================================================================

	/** Currently active overrides — cleaned up each hour */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Schedule")
	TArray<FIronvaleScheduleOverride> ActiveOverrides;

	/** Cached last-resolved activity (for change detection) */
	EIronvaleActivity CachedActivity = EIronvaleActivity::Sleep;

	/** Cached last-resolved location */
	FName CachedLocation;

	// =========================================================================
	// EVENT HANDLERS
	// =========================================================================

	/** Called when the game clock crosses an hour boundary */
	UFUNCTION()
	void HandleHourChanged(int32 NewHour);

	// =========================================================================
	// HELPERS
	// =========================================================================

	/** Re-evaluate the schedule and fire change delegates if the result differs */
	void EvaluateSchedule();

	/** Remove expired overrides from the active list */
	void PurgeExpiredOverrides();

	/** Get the active game state (cached helper) */
	AIronvaleGameState* GetIronvaleGameState() const;

	/** Is the current weather considered adverse? */
	bool IsWeatherAdverse() const;
};
