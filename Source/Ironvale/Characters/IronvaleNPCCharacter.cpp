// =============================================================================
// IronvaleNPCCharacter.cpp — NPC character implementation
// Project Ironvale
// =============================================================================

#include "Characters/IronvaleNPCCharacter.h"
#include "Ironvale.h"
#include "AI/IronvaleScheduleComponent.h"
#include "AI/IronvaleCrimeResponseComponent.h"
#include "Dialogue/IronvaleReputationComponent.h"
#include "Core/IronvaleEventBus.h"

AIronvaleNPCCharacter::AIronvaleNPCCharacter()
{
	// NPC-specific components
	ScheduleComponent = CreateDefaultSubobject<UIronvaleScheduleComponent>(TEXT("ScheduleComponent"));
	CrimeResponseComponent = CreateDefaultSubobject<UIronvaleCrimeResponseComponent>(TEXT("CrimeResponseComponent"));

	// NPCs don't need to tick as frequently as the player
	PrimaryActorTick.TickInterval = 0.1f;
}

void AIronvaleNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogIronvale, Verbose, TEXT("NPC %s (%s) spawned. Faction: %s"),
		*CharacterID.ToString(),
		*DisplayName.ToString(),
		*FactionID.ToString());
}

float AIronvaleNPCCharacter::GetDispositionToward(AActor* OtherActor) const
{
	if (!OtherActor || FactionID.IsNone()) return 0.0f;

	// Look up reputation from the reputation subsystem
	if (UWorld* World = GetWorld())
	{
		if (UIronvaleReputationSubsystem* RepSys = World->GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
		{
			return RepSys->GetReputation(FactionID);
		}
	}

	return 0.0f;
}

bool AIronvaleNPCCharacter::IsHostileToward(AActor* OtherActor) const
{
	// Hostile if reputation is below -50
	return GetDispositionToward(OtherActor) < -50.0f;
}

void AIronvaleNPCCharacter::Die(AActor* Killer)
{
	if (bIsEssential)
	{
		// Essential NPCs are knocked out, not killed
		UE_LOG(LogIronvale, Log, TEXT("Essential NPC %s was knocked out (cannot die)"),
			*CharacterID.ToString());

		// Set health to 1 and stagger instead
		if (HealthComponent)
		{
			// Would set to knocked-out state instead of dead
			// For now, prevent death
		}
		return;
	}

	Super::Die(Killer);
}
