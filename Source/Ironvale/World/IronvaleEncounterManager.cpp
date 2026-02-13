// =============================================================================
// IronvaleEncounterManager.cpp — Random encounter spawning implementation
// Project Ironvale
// =============================================================================

#include "World/IronvaleEncounterManager.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "IronvaleGameInstance.h"
#include "Core/IronvaleEventBus.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// =============================================================================
// SUBSYSTEM LIFECYCLE
// =============================================================================

void UIronvaleEncounterManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogIronvale, Log, TEXT("EncounterManagerSubsystem initialized"));
}

void UIronvaleEncounterManagerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	EncounterTemplates.Empty();
	DailyEncounterLog.Empty();

	Super::Deinitialize();
}

void UIronvaleEncounterManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetSubsystem<UIronvaleEventBus>();

	// Start periodic encounter check timer
	if (bEncountersEnabled && CheckIntervalSeconds > 0.0f)
	{
		InWorld.GetTimerManager().SetTimer(
			CheckTimerHandle,
			this,
			&UIronvaleEncounterManagerSubsystem::PerformEncounterCheck,
			CheckIntervalSeconds,
			true
		);
	}

	UE_LOG(LogIronvale, Log, TEXT("EncounterManager: world begin play, %d templates registered"),
		EncounterTemplates.Num());
}

bool UIronvaleEncounterManagerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// =============================================================================
// ENCOUNTER TABLE MANAGEMENT
// =============================================================================

void UIronvaleEncounterManagerSubsystem::RegisterEncounterTemplate(
	const FIronvaleEncounterTemplate& Template)
{
	if (Template.EncounterID.IsNone())
	{
		UE_LOG(LogIronvale, Warning, TEXT("RegisterEncounterTemplate: template has empty EncounterID"));
		return;
	}

	EncounterTemplates.Add(Template.EncounterID, Template);
	UE_LOG(LogIronvale, Verbose, TEXT("Registered encounter template: %s (%d enemies)"),
		*Template.EncounterID.ToString(), Template.EnemyIDs.Num());
}

void UIronvaleEncounterManagerSubsystem::RegisterEncounterTemplates(
	const TArray<FIronvaleEncounterTemplate>& Templates)
{
	for (const FIronvaleEncounterTemplate& Template : Templates)
	{
		RegisterEncounterTemplate(Template);
	}
}

void UIronvaleEncounterManagerSubsystem::LoadEncounterTable(UDataTable* InTable)
{
	if (!InTable)
	{
		UE_LOG(LogIronvale, Warning, TEXT("LoadEncounterTable: null DataTable"));
		return;
	}

	TArray<FIronvaleEncounterTemplate*> Rows;
	InTable->GetAllRows<FIronvaleEncounterTemplate>(TEXT("LoadEncounterTable"), Rows);

	for (const FIronvaleEncounterTemplate* Row : Rows)
	{
		if (Row)
		{
			RegisterEncounterTemplate(*Row);
		}
	}

	UE_LOG(LogIronvale, Log, TEXT("Loaded %d encounter templates from DataTable %s"),
		Rows.Num(), *InTable->GetName());
}

// =============================================================================
// ENCOUNTER CONTROL
// =============================================================================

void UIronvaleEncounterManagerSubsystem::SetEncountersEnabled(bool bEnabled)
{
	bEncountersEnabled = bEnabled;

	UWorld* World = GetWorld();
	if (!World) return;

	if (bEnabled && !CheckTimerHandle.IsValid() && CheckIntervalSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			CheckTimerHandle,
			this,
			&UIronvaleEncounterManagerSubsystem::PerformEncounterCheck,
			CheckIntervalSeconds,
			true
		);
	}
	else if (!bEnabled && CheckTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	UE_LOG(LogIronvale, Log, TEXT("Encounters %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UIronvaleEncounterManagerSubsystem::ResetDailyEncounters()
{
	DailyEncounterLog.Empty();
	UE_LOG(LogIronvale, Verbose, TEXT("Daily encounter log reset"));
}

bool UIronvaleEncounterManagerSubsystem::TriggerEncounterCheck()
{
	if (!bEncountersEnabled) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	// Check cooldown
	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastEncounterTime < EncounterCooldownSeconds)
	{
		return false;
	}

	if (!IsPlayerTraveling())
	{
		return false;
	}

	// Roll against base chance
	if (FMath::FRand() > BaseEncounterChance)
	{
		return false;
	}

	// Select and spawn an encounter
	FIronvaleEncounterTemplate SelectedTemplate;
	if (!SelectEncounter(SelectedTemplate))
	{
		return false;
	}

	return SpawnEncounter(SelectedTemplate);
}

bool UIronvaleEncounterManagerSubsystem::ForceEncounter(FName EncounterID)
{
	const FIronvaleEncounterTemplate* Template = EncounterTemplates.Find(EncounterID);
	if (!Template)
	{
		UE_LOG(LogIronvale, Warning, TEXT("ForceEncounter: template %s not found"),
			*EncounterID.ToString());
		return false;
	}

	return SpawnEncounter(*Template);
}

// =============================================================================
// ENCOUNTER SELECTION
// =============================================================================

bool UIronvaleEncounterManagerSubsystem::SelectEncounter(
	FIronvaleEncounterTemplate& OutTemplate) const
{
	const UWorld* World = GetWorld();
	if (!World) return false;

	const AIronvaleGameState* GS = World->GetGameState<AIronvaleGameState>();
	if (!GS) return false;

	const EIronvaleTimeOfDay CurrentTime = UIronvaleStatics::HourToTimeOfDay(GS->GetGameTimeHours());
	const EIronvaleBiome CurrentBiome = EIronvaleBiome::Forest; // TODO: get from WorldManager
	const int32 PlayerLevel = 1; // TODO: get from player stats

	// Build weighted candidate list
	TArray<TPair<FName, float>> Candidates;
	float TotalWeight = 0.0f;

	for (const auto& Pair : EncounterTemplates)
	{
		if (IsEncounterEligible(Pair.Value, CurrentBiome, CurrentTime, PlayerLevel))
		{
			Candidates.Add(TPair<FName, float>(Pair.Key, Pair.Value.Weight));
			TotalWeight += Pair.Value.Weight;
		}
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.0f)
	{
		return false;
	}

	// Weighted random selection
	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	FName SelectedID;

	for (const auto& Candidate : Candidates)
	{
		Roll -= Candidate.Value;
		if (Roll <= 0.0f)
		{
			SelectedID = Candidate.Key;
			break;
		}
	}

	if (SelectedID.IsNone() && Candidates.Num() > 0)
	{
		SelectedID = Candidates.Last().Key;
	}

	if (const FIronvaleEncounterTemplate* Found = EncounterTemplates.Find(SelectedID))
	{
		OutTemplate = *Found;
		return true;
	}

	return false;
}

bool UIronvaleEncounterManagerSubsystem::IsEncounterEligible(
	const FIronvaleEncounterTemplate& Template,
	EIronvaleBiome CurrentBiome, EIronvaleTimeOfDay CurrentTime, int32 PlayerLevel) const
{
	if (PlayerLevel < Template.MinPlayerLevel) return false;
	if (Template.MaxPlayerLevel > 0 && PlayerLevel > Template.MaxPlayerLevel) return false;
	if (Template.BiomeFilter.Num() > 0 && !Template.BiomeFilter.Contains(CurrentBiome)) return false;
	if (Template.TimeFilter.Num() > 0 && !Template.TimeFilter.Contains(CurrentTime)) return false;
	if (Template.bUniquePerDay && DailyEncounterLog.Contains(Template.EncounterID)) return false;

	// Quest flag checks
	if (const UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (const UIronvaleGameInstance* IronvaleGI = Cast<UIronvaleGameInstance>(GI))
		{
			if (!Template.RequiredQuestFlag.IsNone() && !IronvaleGI->GetWorldFlag(Template.RequiredQuestFlag))
				return false;
			if (!Template.SuppressedByFlag.IsNone() && IronvaleGI->GetWorldFlag(Template.SuppressedByFlag))
				return false;
		}
	}

	return true;
}

// =============================================================================
// ENCOUNTER SPAWNING
// =============================================================================

bool UIronvaleEncounterManagerSubsystem::SpawnEncounter(const FIronvaleEncounterTemplate& Template)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector SpawnLocation = CalculateSpawnLocation(Template);

	UE_LOG(LogIronvale, Log, TEXT("Spawning encounter '%s' at %s (%d enemies)"),
		*Template.EncounterID.ToString(),
		*SpawnLocation.ToString(),
		Template.EnemyIDs.Num());

	// Record the encounter
	LastEncounterTime = World->GetTimeSeconds();
	DailyEncounterLog.Add(Template.EncounterID);

	// NOTE: Actual actor spawning deferred to Blueprint or spawn subsystem
	// that maps EnemyID -> NPC class via DataTable/AssetManager. This system
	// handles selection, timing, and location only.

	return true;
}

FVector UIronvaleEncounterManagerSubsystem::CalculateSpawnLocation(
	const FIronvaleEncounterTemplate& Template) const
{
	const APawn* Player = GetPlayerPawn();
	if (!Player)
	{
		return FVector::ZeroVector;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	const FVector PlayerForward = Player->GetActorForwardVector();

	const float Distance = FMath::FRandRange(Template.MinSpawnDistance, Template.MaxSpawnDistance);
	const float AngleOffset = FMath::FRandRange(-45.0f, 45.0f);

	const FRotator RandomRotation(0.0f, PlayerForward.Rotation().Yaw + AngleOffset, 0.0f);
	const FVector Offset = RandomRotation.Vector() * Distance;

	return PlayerLocation + Offset;
}

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

void UIronvaleEncounterManagerSubsystem::PerformEncounterCheck()
{
	TriggerEncounterCheck();
}

bool UIronvaleEncounterManagerSubsystem::IsPlayerTraveling() const
{
	const APawn* Player = GetPlayerPawn();
	if (!Player) return false;

	const float Speed = Player->GetVelocity().Size();
	return Speed >= TravelSpeedThreshold;
}

APawn* UIronvaleEncounterManagerSubsystem::GetPlayerPawn() const
{
	const UWorld* World = GetWorld();
	if (!World) return nullptr;

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return nullptr;

	return PC->GetPawn();
}
