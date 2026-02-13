// =============================================================================
// IronvaleSaveManager.cpp — Save/load orchestration implementation
// Project Ironvale
// =============================================================================

#include "World/IronvaleSaveManager.h"
#include "World/IronvaleSaveGame.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "IronvaleGameInstance.h"
#include "Quests/IronvaleQuestManager.h"
#include "Dialogue/IronvaleReputationComponent.h"
#include "Characters/IronvalePlayerCharacter.h"
#include "Combat/IronvaleHealthComponent.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Inventory/IronvaleEquipmentComponent.h"
#include "Needs/IronvaleNeedsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

const FString UIronvaleSaveSubsystem::ManualSlotPrefix = TEXT("Save_");
const FString UIronvaleSaveSubsystem::AutoSaveSlotName = TEXT("AutoSave");
const FString UIronvaleSaveSubsystem::QuickSaveSlotName = TEXT("QuickSave");

// =============================================================================
// INITIALIZATION
// =============================================================================

void UIronvaleSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogIronvale, Log, TEXT("SaveSubsystem initialized (version %d)"), CurrentSaveVersion);
}

void UIronvaleSaveSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// =============================================================================
// SAVE OPERATIONS
// =============================================================================

bool UIronvaleSaveSubsystem::SaveToSlot(const FString& SlotName)
{
	UIronvaleSaveGame* SaveGame = GatherSaveData();
	if (!SaveGame)
	{
		UE_LOG(LogIronvale, Error, TEXT("SaveToSlot: failed to gather save data"));
		OnSaveComplete.Broadcast(false);
		return false;
	}

	// Set slot metadata
	SaveGame->SlotInfo.SlotName = SlotName;
	SaveGame->SlotInfo.SaveTimestamp = FDateTime::Now();

	if (const UIronvaleGameInstance* GI = Cast<UIronvaleGameInstance>(GetGameInstance()))
	{
		SaveGame->SlotInfo.PlayTimeSeconds = GI->GetTotalPlayTimeSeconds();
	}

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);

	if (bSuccess)
	{
		UE_LOG(LogIronvale, Log, TEXT("Game saved to slot '%s'"), *SlotName);
	}
	else
	{
		UE_LOG(LogIronvale, Error, TEXT("Failed to save game to slot '%s'"), *SlotName);
	}

	OnSaveComplete.Broadcast(bSuccess);
	return bSuccess;
}

bool UIronvaleSaveSubsystem::QuickSave()
{
	return SaveToSlot(QuickSaveSlotName);
}

bool UIronvaleSaveSubsystem::AutoSave()
{
	return SaveToSlot(AutoSaveSlotName);
}

// =============================================================================
// LOAD OPERATIONS
// =============================================================================

bool UIronvaleSaveSubsystem::LoadFromSlot(const FString& SlotName)
{
	if (!DoesSaveExist(SlotName))
	{
		UE_LOG(LogIronvale, Warning, TEXT("LoadFromSlot: slot '%s' does not exist"), *SlotName);
		OnLoadComplete.Broadcast(false);
		return false;
	}

	USaveGame* LoadedData = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	UIronvaleSaveGame* SaveGame = Cast<UIronvaleSaveGame>(LoadedData);

	if (!SaveGame)
	{
		UE_LOG(LogIronvale, Error, TEXT("LoadFromSlot: failed to deserialize slot '%s'"), *SlotName);
		OnLoadComplete.Broadcast(false);
		return false;
	}

	// Validate and migrate if needed
	if (!ValidateAndMigrate(SaveGame))
	{
		UE_LOG(LogIronvale, Error, TEXT("LoadFromSlot: save validation failed for slot '%s'"), *SlotName);
		OnLoadComplete.Broadcast(false);
		return false;
	}

	const bool bSuccess = RestoreFromSaveData(SaveGame);

	if (bSuccess)
	{
		UE_LOG(LogIronvale, Log, TEXT("Game loaded from slot '%s' (version %d, day %d)"),
			*SlotName, SaveGame->SaveVersion, SaveGame->WorldState.DayCount);
	}
	else
	{
		UE_LOG(LogIronvale, Error, TEXT("Failed to restore game from slot '%s'"), *SlotName);
	}

	OnLoadComplete.Broadcast(bSuccess);
	return bSuccess;
}

bool UIronvaleSaveSubsystem::QuickLoad()
{
	return LoadFromSlot(QuickSaveSlotName);
}

// =============================================================================
// SLOT MANAGEMENT
// =============================================================================

bool UIronvaleSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UIronvaleSaveSubsystem::DeleteSave(const FString& SlotName)
{
	if (!DoesSaveExist(SlotName))
	{
		return false;
	}

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	if (bDeleted)
	{
		UE_LOG(LogIronvale, Log, TEXT("Deleted save slot '%s'"), *SlotName);
	}
	return bDeleted;
}

bool UIronvaleSaveSubsystem::GetSlotInfo(const FString& SlotName, FIronvaleSaveSlotInfo& OutInfo)
{
	if (!DoesSaveExist(SlotName))
	{
		return false;
	}

	USaveGame* LoadedData = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	const UIronvaleSaveGame* SaveGame = Cast<UIronvaleSaveGame>(LoadedData);
	if (!SaveGame)
	{
		return false;
	}

	OutInfo = SaveGame->SlotInfo;
	return true;
}

TArray<FIronvaleSaveSlotInfo> UIronvaleSaveSubsystem::GetAllSaveSlotInfos()
{
	TArray<FIronvaleSaveSlotInfo> Results;

	// Check manual slots
	for (int32 i = 0; i < MaxManualSlots; ++i)
	{
		const FString SlotName = ManualSlotPrefix + FString::FromInt(i);
		FIronvaleSaveSlotInfo Info;
		if (GetSlotInfo(SlotName, Info))
		{
			Results.Add(Info);
		}
	}

	// Check auto-save
	FIronvaleSaveSlotInfo AutoInfo;
	if (GetSlotInfo(AutoSaveSlotName, AutoInfo))
	{
		Results.Add(AutoInfo);
	}

	// Check quicksave
	FIronvaleSaveSlotInfo QuickInfo;
	if (GetSlotInfo(QuickSaveSlotName, QuickInfo))
	{
		Results.Add(QuickInfo);
	}

	return Results;
}

// =============================================================================
// STATE GATHERING
// =============================================================================

UIronvaleSaveGame* UIronvaleSaveSubsystem::GatherSaveData()
{
	UIronvaleSaveGame* SaveGame = NewObject<UIronvaleSaveGame>();
	SaveGame->SaveVersion = CurrentSaveVersion;

	GatherPlayerData(SaveGame);
	GatherWorldData(SaveGame);
	GatherQuestData(SaveGame);
	GatherNPCData(SaveGame);

	return SaveGame;
}

void UIronvaleSaveSubsystem::GatherPlayerData(UIronvaleSaveGame* SaveGame)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	APawn* PlayerPawn = PC->GetPawn();
	FIronvaleSavedPlayerData& Data = SaveGame->PlayerData;

	Data.Location = PlayerPawn->GetActorLocation();
	Data.Rotation = PlayerPawn->GetActorRotation();

	// Health
	if (const UIronvaleHealthComponent* HealthComp = PlayerPawn->FindComponentByClass<UIronvaleHealthComponent>())
	{
		Data.Health = HealthComp->GetCurrentHealth();
		Data.MaxHealth = HealthComp->GetMaxHealth();
	}

	// Stamina
	if (const UIronvaleStaminaComponent* StaminaComp = PlayerPawn->FindComponentByClass<UIronvaleStaminaComponent>())
	{
		Data.Stamina = StaminaComp->GetCurrentStamina();
		Data.MaxStamina = StaminaComp->GetMaxStamina();
	}

	// Inventory
	if (const UIronvaleInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UIronvaleInventoryComponent>())
	{
		Data.Gold = InvComp->GetGold();
		Data.InventoryItems = InvComp->ToSaveData();
	}

	// Equipment
	if (const UIronvaleEquipmentComponent* EquipComp = PlayerPawn->FindComponentByClass<UIronvaleEquipmentComponent>())
	{
		Data.Equipment = EquipComp->ToSaveData();
	}

	// Needs
	if (const UIronvaleNeedsComponent* NeedsComp = PlayerPawn->FindComponentByClass<UIronvaleNeedsComponent>())
	{
		Data.Needs.Hunger = NeedsComp->GetNeedValue(EIronvaleNeedType::Hunger);
		Data.Needs.Thirst = NeedsComp->GetNeedValue(EIronvaleNeedType::Thirst);
		Data.Needs.Fatigue = NeedsComp->GetNeedValue(EIronvaleNeedType::Fatigue);
		Data.Needs.Cleanliness = NeedsComp->GetNeedValue(EIronvaleNeedType::Cleanliness);
	}

	// Store player location name for slot info
	SaveGame->SlotInfo.PlayerLocation = TEXT("Unknown");
	SaveGame->SlotInfo.DayCount = SaveGame->WorldState.DayCount;
}

void UIronvaleSaveSubsystem::GatherWorldData(UIronvaleSaveGame* SaveGame)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	FIronvaleSavedWorldState& State = SaveGame->WorldState;

	// Game time and weather from GameState
	if (const AIronvaleGameState* GS = World->GetGameState<AIronvaleGameState>())
	{
		State.GameTimeHours = GS->GetGameTimeHours();
		State.DayCount = GS->GetDayCount();
		State.CurrentWeather = GS->GetCurrentWeather();
	}

	// World flags from GameInstance
	if (const UIronvaleGameInstance* GI = Cast<UIronvaleGameInstance>(GetGameInstance()))
	{
		State.WorldFlags = GI->GetAllWorldFlags();
	}

	// Reputation from ReputationSubsystem
	if (const UIronvaleReputationSubsystem* RepSys = GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
	{
		State.Reputations = RepSys->GetAllReputations();
	}
}

void UIronvaleSaveSubsystem::GatherQuestData(UIronvaleSaveGame* SaveGame)
{
	if (const UIronvaleQuestSubsystem* QuestSys = GetGameInstance()->GetSubsystem<UIronvaleQuestSubsystem>())
	{
		SaveGame->WorldState.Quests = QuestSys->ExportQuestStates();
	}
}

void UIronvaleSaveSubsystem::GatherNPCData(UIronvaleSaveGame* SaveGame)
{
	// NPC state gathering is done by iterating all AIronvaleNPCCharacter
	// instances in the world and exporting their state. This is deferred
	// to the NPC system's own export logic.
	// SaveGame->WorldState.NPCStates populated here when NPC export is connected.
}

// =============================================================================
// STATE RESTORATION
// =============================================================================

bool UIronvaleSaveSubsystem::RestoreFromSaveData(UIronvaleSaveGame* SaveGame)
{
	if (!SaveGame) return false;

	RestoreWorldData(SaveGame);
	RestoreQuestData(SaveGame);
	RestorePlayerData(SaveGame);
	RestoreNPCData(SaveGame);

	return true;
}

void UIronvaleSaveSubsystem::RestorePlayerData(const UIronvaleSaveGame* SaveGame)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	APawn* PlayerPawn = PC->GetPawn();
	const FIronvaleSavedPlayerData& Data = SaveGame->PlayerData;

	// Teleport player
	PlayerPawn->SetActorLocationAndRotation(Data.Location, Data.Rotation);

	// Health
	if (UIronvaleHealthComponent* HealthComp = PlayerPawn->FindComponentByClass<UIronvaleHealthComponent>())
	{
		HealthComp->SetMaxHealth(Data.MaxHealth);
		HealthComp->SetHealth(Data.Health);
	}

	// Stamina
	if (UIronvaleStaminaComponent* StaminaComp = PlayerPawn->FindComponentByClass<UIronvaleStaminaComponent>())
	{
		StaminaComp->SetMaxStamina(Data.MaxStamina);
		StaminaComp->SetStamina(Data.Stamina);
	}

	// Inventory
	UIronvaleInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UIronvaleInventoryComponent>();
	if (InvComp)
	{
		InvComp->LoadFromSaveData(Data.InventoryItems, nullptr);
		InvComp->RemoveGold(InvComp->GetGold());
		InvComp->AddGold(Data.Gold);
	}

	// Equipment
	if (UIronvaleEquipmentComponent* EquipComp = PlayerPawn->FindComponentByClass<UIronvaleEquipmentComponent>())
	{
		EquipComp->LoadFromSaveData(Data.Equipment, InvComp);
	}

	// Needs
	if (UIronvaleNeedsComponent* NeedsComp = PlayerPawn->FindComponentByClass<UIronvaleNeedsComponent>())
	{
		NeedsComp->SetNeedValue(EIronvaleNeedType::Hunger, Data.Needs.Hunger);
		NeedsComp->SetNeedValue(EIronvaleNeedType::Thirst, Data.Needs.Thirst);
		NeedsComp->SetNeedValue(EIronvaleNeedType::Fatigue, Data.Needs.Fatigue);
		NeedsComp->SetNeedValue(EIronvaleNeedType::Cleanliness, Data.Needs.Cleanliness);
	}
}

void UIronvaleSaveSubsystem::RestoreWorldData(const UIronvaleSaveGame* SaveGame)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	const FIronvaleSavedWorldState& State = SaveGame->WorldState;

	// Game time and weather
	if (AIronvaleGameState* GS = World->GetGameState<AIronvaleGameState>())
	{
		GS->SetGameTimeFromSave(State.GameTimeHours, State.DayCount);
		GS->SetWeather(State.CurrentWeather);
	}

	// World flags
	if (UIronvaleGameInstance* GI = Cast<UIronvaleGameInstance>(GetGameInstance()))
	{
		GI->LoadWorldFlags(State.WorldFlags);
	}

	// Reputation
	if (UIronvaleReputationSubsystem* RepSys = GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
	{
		RepSys->LoadReputations(State.Reputations);
	}
}

void UIronvaleSaveSubsystem::RestoreQuestData(const UIronvaleSaveGame* SaveGame)
{
	if (UIronvaleQuestSubsystem* QuestSys = GetGameInstance()->GetSubsystem<UIronvaleQuestSubsystem>())
	{
		QuestSys->ImportQuestStates(SaveGame->WorldState.Quests);
	}
}

void UIronvaleSaveSubsystem::RestoreNPCData(const UIronvaleSaveGame* SaveGame)
{
	// NPC state restoration deferred to NPC system's import logic
	// Iterate SaveGame->WorldState.NPCStates and restore each NPC
}

bool UIronvaleSaveSubsystem::ValidateAndMigrate(UIronvaleSaveGame* SaveGame)
{
	if (!SaveGame) return false;

	if (SaveGame->SaveVersion > CurrentSaveVersion)
	{
		UE_LOG(LogIronvale, Error,
			TEXT("Save version %d is newer than current version %d — cannot load"),
			SaveGame->SaveVersion, CurrentSaveVersion);
		return false;
	}

	// Apply migrations for older save versions
	if (SaveGame->SaveVersion < CurrentSaveVersion)
	{
		UE_LOG(LogIronvale, Log, TEXT("Migrating save from version %d to %d"),
			SaveGame->SaveVersion, CurrentSaveVersion);

		// Version-specific migration logic goes here
		// Example: if (SaveGame->SaveVersion < 2) { /* migrate v1 -> v2 */ }

		SaveGame->SaveVersion = CurrentSaveVersion;
	}

	return true;
}
