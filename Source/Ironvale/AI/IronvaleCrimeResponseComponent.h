// =============================================================================
// IronvaleCrimeResponseComponent.h — NPC crime witness and response system
// Project Ironvale
//
// Subscribes to EventBus::OnCrimeCommitted. When a crime event fires, this
// component checks whether the owning NPC can actually witness it (line of
// sight + distance). If witnessed, the NPC's response depends on archetype:
//
//   Guard     -> Pursue and attack the criminal
//   Merchant  -> Flee to safety
//   Villager  -> Report the crime to the nearest guard
//   Noble     -> Report (higher priority) and demand arrest
//   Priest    -> Verbally condemn, report to guard
//   Child     -> Flee
//   Bandit    -> Ignore (they don't care about crime)
//   Traveler  -> Flee
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleCrimeResponseComponent.generated.h"

class UIronvaleEventBus;
class AIronvaleNPCCharacter;

/**
 * The type of response an NPC takes when witnessing a crime.
 */
UENUM(BlueprintType)
enum class EIronvaleCrimeResponse : uint8
{
	None     UMETA(DisplayName = "None"),
	Pursue   UMETA(DisplayName = "Pursue"),     // Guards: chase and engage
	Flee     UMETA(DisplayName = "Flee"),        // Merchants, children, travelers
	Report   UMETA(DisplayName = "Report"),      // Villagers, nobles, priests: find a guard
	Ignore   UMETA(DisplayName = "Ignore")       // Bandits, or crimes too far away
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleCrimeWitnessedSignature,
	AActor*, Criminal, FName, CrimeType, EIronvaleCrimeResponse, Response);

/**
 * UIronvaleCrimeResponseComponent
 *
 * Manages an NPC's awareness of and reaction to crimes. Subscribes to the
 * global crime event and performs local visibility/distance checks to determine
 * if this specific NPC actually witnessed the crime.
 */
UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleCrimeResponseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleCrimeResponseComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Maximum distance (cm) at which this NPC can witness a crime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Crime",
		meta = (ClampMin = "0.0"))
	float WitnessRange = 2000.0f;

	/**
	 * Whether line-of-sight is required to witness a crime.
	 * If false, only distance is checked (useful for hearing-based witnessing).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Crime")
	bool bRequireLineOfSight = true;

	/**
	 * Duration (real seconds) that the NPC remembers having witnessed a crime.
	 * After this time the bHasWitnessedCrime flag is cleared. Set to 0 for
	 * infinite memory.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Crime",
		meta = (ClampMin = "0.0"))
	float CrimeMemoryDuration = 120.0f;

	// =========================================================================
	// RUNTIME STATE (read-only externally)
	// =========================================================================

	/** Whether this NPC has witnessed a crime that hasn't been resolved yet */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Crime")
	bool GetHasWitnessedCrime() const { return bHasWitnessedCrime; }

	/** The type of crime that was witnessed */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Crime")
	FName GetWitnessedCrimeType() const { return WitnessedCrimeType; }

	/** The actor that committed the crime */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Crime")
	AActor* GetCriminalActor() const { return CriminalActor.Get(); }

	/** Get the response type for this NPC's archetype */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Crime")
	EIronvaleCrimeResponse GetResponseType() const;

	// =========================================================================
	// ACTIONS
	// =========================================================================

	/**
	 * Manually trigger a crime witness event (e.g. from a dialogue action
	 * or scripted sequence).
	 *
	 * @param Criminal   The actor that committed the crime
	 * @param CrimeType  Crime identifier (e.g. "Theft", "Murder", "Trespassing")
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Crime")
	void WitnessCrime(AActor* Criminal, FName CrimeType);

	/**
	 * Attempt to report the witnessed crime to the nearest guard NPC.
	 * Returns true if a guard was found and notified.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Crime")
	bool ReportCrime();

	/** Clear the crime witness state (e.g. criminal paid a fine, bribed, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Crime")
	void ForgetCrime();

	// =========================================================================
	// EVENTS
	// =========================================================================

	/** Fired when this NPC witnesses a crime */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Crime")
	FOnIronvaleCrimeWitnessedSignature OnCrimeWitnessed;

protected:
	// =========================================================================
	// STATE
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Crime")
	bool bHasWitnessedCrime = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Crime")
	FName WitnessedCrimeType;

	/** Weak pointer to avoid preventing GC of the criminal actor */
	TWeakObjectPtr<AActor> CriminalActor;

	/** Timer handle for crime memory expiration */
	FTimerHandle CrimeMemoryTimerHandle;

	// =========================================================================
	// EVENT HANDLERS
	// =========================================================================

	/** EventBus callback when any crime is committed in the world */
	UFUNCTION()
	void HandleCrimeCommitted(AActor* Criminal, FName CrimeType, AActor* Witness);

	// =========================================================================
	// HELPERS
	// =========================================================================

	/**
	 * Check whether this NPC can perceive the crime location/criminal.
	 * Performs distance check and optional line-of-sight trace.
	 */
	bool CanWitnessCrime(AActor* Criminal) const;

	/** Get the owning NPC character (casts the owner) */
	AIronvaleNPCCharacter* GetOwnerNPC() const;

	/** Determine the crime response based on the NPC's archetype */
	EIronvaleCrimeResponse DetermineResponse(EIronvaleNPCArchetype Archetype) const;

	/** Guard-specific: inject a pursue/combat override into the schedule */
	void HandleGuardPursue(AActor* Criminal);

	/** Flee-specific: inject a flee override into the schedule */
	void HandleFlee();

	/** Called when the crime memory timer expires */
	void HandleCrimeMemoryExpired();
};
