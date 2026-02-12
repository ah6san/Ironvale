// =============================================================================
// IronvaleTestLevelBuilder.cpp — Procedural test level generator
// Project Ironvale
// =============================================================================

#include "World/IronvaleTestLevelBuilder.h"
#include "Ironvale.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIronvaleTestLevelBuilder::AIronvaleTestLevelBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AIronvaleTestLevelBuilder::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogIronvale, Log, TEXT("TestLevelBuilder: Generating test arena (%.0f x %.0f)"), ArenaSize, ArenaSize);

	SpawnFloor();
	SpawnWalls();
	SpawnLighting();
	SpawnObstacles();

	UE_LOG(LogIronvale, Log, TEXT("TestLevelBuilder: Test arena generation complete"));
}

void AIronvaleTestLevelBuilder::SpawnFloor()
{
	const float FloorScale = ArenaSize / 100.0f; // Cube mesh is 100x100x100
	SpawnCube(
		FVector(0.0f, 0.0f, -WallThickness * 0.5f),
		FVector(FloorScale, FloorScale, WallThickness / 100.0f),
		FLinearColor(0.25f, 0.22f, 0.18f) // Dark stone color
	);
}

void AIronvaleTestLevelBuilder::SpawnWalls()
{
	const float HalfArena = ArenaSize * 0.5f;
	const float WallScale = ArenaSize / 100.0f;
	const float HeightScale = WallHeight / 100.0f;
	const float ThickScale = WallThickness / 100.0f;
	const float WallZ = WallHeight * 0.5f;
	const FLinearColor WallColor(0.35f, 0.30f, 0.25f);

	// North wall (+Y)
	SpawnCube(FVector(0.0f, HalfArena, WallZ), FVector(WallScale, ThickScale, HeightScale), WallColor);
	// South wall (-Y)
	SpawnCube(FVector(0.0f, -HalfArena, WallZ), FVector(WallScale, ThickScale, HeightScale), WallColor);
	// East wall (+X)
	SpawnCube(FVector(HalfArena, 0.0f, WallZ), FVector(ThickScale, WallScale, HeightScale), WallColor);
	// West wall (-X)
	SpawnCube(FVector(-HalfArena, 0.0f, WallZ), FVector(ThickScale, WallScale, HeightScale), WallColor);
}

void AIronvaleTestLevelBuilder::SpawnLighting()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Directional light (sun)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AActor* SunActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams))
	{
		UDirectionalLightComponent* SunLight = NewObject<UDirectionalLightComponent>(SunActor, TEXT("SunLight"));
		SunLight->SetWorldRotation(FRotator(-50.0f, -30.0f, 0.0f));
		SunLight->Intensity = 3.0f;
		SunLight->LightColor = FColor(255, 245, 230); // Warm sunlight
		SunLight->RegisterComponent();
		SunActor->SetRootComponent(SunLight);
	}

	// Sky light for ambient fill
	if (AActor* SkyActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams))
	{
		USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(SkyActor, TEXT("SkyLight"));
		SkyLight->SetWorldLocation(FVector(0.0f, 0.0f, 1000.0f));
		SkyLight->Intensity = 1.0f;
		SkyLight->RegisterComponent();
		SkyActor->SetRootComponent(SkyLight);
	}
}

void AIronvaleTestLevelBuilder::SpawnObstacles()
{
	const FLinearColor ObstacleColor(0.4f, 0.15f, 0.1f); // Reddish-brown

	// A few pillars for cover/navigation testing
	const float PillarHeight = 200.0f;
	const float PillarSize = 80.0f;
	const float PScale = PillarSize / 100.0f;
	const float HScale = PillarHeight / 100.0f;

	SpawnCube(FVector(500.0f, 500.0f, PillarHeight * 0.5f), FVector(PScale, PScale, HScale), ObstacleColor);
	SpawnCube(FVector(-500.0f, 500.0f, PillarHeight * 0.5f), FVector(PScale, PScale, HScale), ObstacleColor);
	SpawnCube(FVector(500.0f, -500.0f, PillarHeight * 0.5f), FVector(PScale, PScale, HScale), ObstacleColor);
	SpawnCube(FVector(-500.0f, -500.0f, PillarHeight * 0.5f), FVector(PScale, PScale, HScale), ObstacleColor);

	// Central platform for elevation testing
	SpawnCube(FVector(0.0f, 0.0f, 25.0f), FVector(2.0f, 2.0f, 0.5f), FLinearColor(0.3f, 0.28f, 0.22f));
}

AActor* AIronvaleTestLevelBuilder::SpawnCube(FVector Location, FVector Scale, FLinearColor Color)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* CubeActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, Location), SpawnParams);
	if (!CubeActor) return nullptr;

	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(CubeActor, TEXT("Mesh"));

	// Use engine built-in cube mesh
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh)
	{
		MeshComp->SetStaticMesh(CubeMesh);
	}

	MeshComp->SetWorldScale3D(Scale);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComp->RegisterComponent();
	CubeActor->SetRootComponent(MeshComp);

	// Create a colored material instance
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, MeshComp);
		DynMat->SetVectorParameterValue(TEXT("Color"), Color);
		MeshComp->SetMaterial(0, DynMat);
	}

	return CubeActor;
}
