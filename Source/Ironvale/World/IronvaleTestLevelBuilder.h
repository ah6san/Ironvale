// =============================================================================
// IronvaleTestLevelBuilder.h — Procedural test level generator
// Project Ironvale
//
// Spawns a playable test environment at runtime so the game can be played
// without any editor-created map assets. Generates:
//   - Floor and boundary walls (BSP-style static meshes)
//   - Directional light + sky light for basic illumination
//   - Player start point
//   - Optional test obstacles for movement/combat testing
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronvaleTestLevelBuilder.generated.h"

UCLASS()
class IRONVALE_API AIronvaleTestLevelBuilder : public AActor
{
	GENERATED_BODY()

public:
	AIronvaleTestLevelBuilder();

	virtual void BeginPlay() override;

protected:
	/** Size of the arena floor in units */
	UPROPERTY(EditDefaultsOnly, Category = "Ironvale|TestLevel")
	float ArenaSize = 3000.0f;

	/** Height of boundary walls */
	UPROPERTY(EditDefaultsOnly, Category = "Ironvale|TestLevel")
	float WallHeight = 400.0f;

	/** Thickness of walls and floor */
	UPROPERTY(EditDefaultsOnly, Category = "Ironvale|TestLevel")
	float WallThickness = 50.0f;

private:
	void SpawnFloor();
	void SpawnWalls();
	void SpawnLighting();
	void SpawnObstacles();

	/** Helper: spawn a scaled cube at a world location */
	AActor* SpawnCube(FVector Location, FVector Scale, FLinearColor Color);
};
