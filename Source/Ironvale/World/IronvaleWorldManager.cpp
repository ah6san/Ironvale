// =============================================================================
// IronvaleWorldManager.cpp — Open world region tracking implementation
// Project Ironvale
// =============================================================================

#include "World/IronvaleWorldManager.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "Engine/DataTable.h"

// =============================================================================
// SUBSYSTEM LIFECYCLE
// =============================================================================

void UIronvaleWorldManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register tick via FTSTicker
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaSeconds) -> bool
		{
			Tick(DeltaSeconds);
			return true;
		}),
		1.0f // Check region every second
	);

	UE_LOG(LogIronvale, Log, TEXT("WorldManagerSubsystem initialized"));
}

void UIronvaleWorldManagerSubsystem::Deinitialize()
{
	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AILODTimerHandle);
	}

	RegionRegistry.Empty();
	DiscoveredLocations.Empty();

	Super::Deinitialize();
}

void UIronvaleWorldManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetSubsystem<UIronvaleEventBus>();

	// Start AI LOD update timer
	if (AILODUpdateInterval > 0.0f)
	{
		InWorld.GetTimerManager().SetTimer(
			AILODTimerHandle,
			this,
			&UIronvaleWorldManagerSubsystem::UpdateNPCAILOD,
			AILODUpdateInterval,
			true
		);
	}

	// Initial region check
	UpdatePlayerRegion();

	UE_LOG(LogIronvale, Log, TEXT("WorldManager: world begin play, %d regions registered"),
		RegionRegistry.Num());
}

bool UIronvaleWorldManagerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// =============================================================================
// TICK
// =============================================================================

void UIronvaleWorldManagerSubsystem::Tick(float DeltaSeconds)
{
	UpdatePlayerRegion();
}

// =============================================================================
// REGION REGISTRATION
// =============================================================================

void UIronvaleWorldManagerSubsystem::RegisterRegion(UIronvaleRegionDataAsset* RegionAsset)
{
	if (!RegionAsset)
	{
		UE_LOG(LogIronvale, Warning, TEXT("RegisterRegion: null asset"));
		return;
	}

	const FName RegionID = RegionAsset->GetRegionID();
	if (RegionID.IsNone())
	{
		UE_LOG(LogIronvale, Warning, TEXT("RegisterRegion: asset has empty RegionID"));
		return;
	}

	RegionRegistry.Add(RegionID, RegionAsset);
	UE_LOG(LogIronvale, Verbose, TEXT("Registered region: %s (%s)"),
		*RegionID.ToString(), *RegionAsset->GetDisplayName().ToString());
}

void UIronvaleWorldManagerSubsystem::RegisterRegions(
	const TArray<UIronvaleRegionDataAsset*>& RegionAssets)
{
	for (UIronvaleRegionDataAsset* Asset : RegionAssets)
	{
		RegisterRegion(Asset);
	}
}

TArray<FName> UIronvaleWorldManagerSubsystem::GetRegisteredRegionIDs() const
{
	TArray<FName> Result;
	RegionRegistry.GetKeys(Result);
	return Result;
}

// =============================================================================
// REGION QUERIES
// =============================================================================

bool UIronvaleWorldManagerSubsystem::GetCurrentRegionData(FIronvaleRegionData& OutData) const
{
	return GetRegionData(CurrentRegionID, OutData);
}

bool UIronvaleWorldManagerSubsystem::GetRegionData(FName RegionID, FIronvaleRegionData& OutData) const
{
	if (const TObjectPtr<UIronvaleRegionDataAsset>* Found = RegionRegistry.Find(RegionID))
	{
		if (*Found)
		{
			OutData = (*Found)->RegionData;
			return true;
		}
	}
	return false;
}

EIronvaleBiome UIronvaleWorldManagerSubsystem::GetCurrentBiome() const
{
	FIronvaleRegionData Data;
	if (GetCurrentRegionData(Data))
	{
		return Data.Biome;
	}
	return EIronvaleBiome::Forest;
}

bool UIronvaleWorldManagerSubsystem::IsInTown() const
{
	FIronvaleRegionData Data;
	if (GetCurrentRegionData(Data))
	{
		return Data.bIsTown;
	}
	return false;
}

FName UIronvaleWorldManagerSubsystem::GetCurrentFaction() const
{
	FIronvaleRegionData Data;
	if (GetCurrentRegionData(Data))
	{
		return Data.AssociatedFaction;
	}
	return NAME_None;
}

FName UIronvaleWorldManagerSubsystem::GetCurrentEncounterTableID() const
{
	FIronvaleRegionData Data;
	if (GetCurrentRegionData(Data))
	{
		return Data.EncounterTableID;
	}
	return NAME_None;
}

FVector2D UIronvaleWorldManagerSubsystem::GetCurrentDifficultyRange() const
{
	FIronvaleRegionData Data;
	if (GetCurrentRegionData(Data))
	{
		return Data.DifficultyRange;
	}
	return FVector2D(1.0, 5.0);
}

// =============================================================================
// POINT OF INTEREST
// =============================================================================

void UIronvaleWorldManagerSubsystem::DiscoverLocation(FName LocationID)
{
	if (LocationID.IsNone()) return;

	if (!DiscoveredLocations.Contains(LocationID))
	{
		DiscoveredLocations.Add(LocationID);

		if (CachedEventBus)
		{
			CachedEventBus->OnLocationReached.Broadcast(nullptr, LocationID);
		}

		UE_LOG(LogIronvale, Log, TEXT("Location discovered: %s"), *LocationID.ToString());
	}
}

bool UIronvaleWorldManagerSubsystem::IsLocationDiscovered(FName LocationID) const
{
	return DiscoveredLocations.Contains(LocationID);
}

TArray<FName> UIronvaleWorldManagerSubsystem::GetDiscoveredLocations() const
{
	return DiscoveredLocations.Array();
}

// =============================================================================
// INTERNAL
// =============================================================================

void UIronvaleWorldManagerSubsystem::UpdatePlayerRegion()
{
	const FVector PlayerPos = GetPlayerPosition();
	if (PlayerPos.IsZero()) return;

	// Check all registered regions to find which one contains the player
	FName NewRegionID = NAME_None;

	for (const auto& Pair : RegionRegistry)
	{
		if (Pair.Value)
		{
			// Simple distance-based region check
			// In a full implementation, this would use region bounds volumes
			const FIronvaleRegionData& Data = Pair.Value->RegionData;
			// For now, any registered region can match if we have region triggers
			// This is a simplified fallback
		}
	}

	// If the player moved to a different region, handle the transition
	if (!NewRegionID.IsNone() && NewRegionID != CurrentRegionID)
	{
		HandleRegionChange(CurrentRegionID, NewRegionID);
	}
}

void UIronvaleWorldManagerSubsystem::HandleRegionChange(FName OldRegionID, FName NewRegionID)
{
	UE_LOG(LogIronvale, Log, TEXT("Region changed: %s -> %s"),
		OldRegionID.IsNone() ? TEXT("None") : *OldRegionID.ToString(),
		*NewRegionID.ToString());

	CurrentRegionID = NewRegionID;

	// Broadcast region change
	OnRegionChanged.Broadcast(OldRegionID, NewRegionID);

	// Auto-discover the new region's associated locations
	FIronvaleRegionData Data;
	if (GetRegionData(NewRegionID, Data))
	{
		// Discover the region's main POIs as the player enters
		for (const FName& SubRegion : Data.SubRegions)
		{
			// Sub-regions are not auto-discovered, only when explicitly visited
		}
	}
}

void UIronvaleWorldManagerSubsystem::UpdateNPCAILOD()
{
	// NPC AI LOD: adjust behavior tree complexity based on distance from player
	// Full behavior tree when near, simplified schedule-tick when distant,
	// frozen when very far or streamed out.
	// Implementation deferred to per-NPC logic in AIronvaleNPCCharacter and
	// AIronvaleAIController, which query this subsystem's distance thresholds.

	const FVector PlayerPos = GetPlayerPosition();
	if (PlayerPos.IsZero()) return;

	// The actual AI LOD logic is handled by each NPC's AI controller, which
	// queries SimplifiedAIDistance and FrozenAIDistance from this subsystem.
	// This method exists as a hook for batch updates if needed.
}

FVector UIronvaleWorldManagerSubsystem::GetPlayerPosition() const
{
	const UWorld* World = GetWorld();
	if (!World) return FVector::ZeroVector;

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return FVector::ZeroVector;

	return PC->GetPawn()->GetActorLocation();
}
