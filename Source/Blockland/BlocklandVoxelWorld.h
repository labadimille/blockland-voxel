#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlocklandTypes.h"
#include "BlocklandVoxelWorld.generated.h"

class ABlocklandCharacter;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMesh;

UCLASS()
class BLOCKLAND_API ABlocklandVoxelWorld : public AActor
{
    GENERATED_BODY()

public:
    ABlocklandVoxelWorld();

    virtual void Tick(float DeltaSeconds) override;

    void Initialize(
        ABlocklandCharacter* InPlayer,
        int32 InSeed,
        const TArray<FIntVector>& InRemovedCells,
        const TArray<FBlocklandPlacedBlock>& InPlacedCells);

    int32 GetSurfaceHeight(int32 X, int32 Y) const;
    bool RemoveBlock(const FHitResult& Hit, EBlocklandBlock& OutType);
    bool PlaceBlock(const FHitResult& Hit, EBlocklandBlock Type, const FVector& PlayerLocation);
    void ExportState(TArray<FIntVector>& OutRemovedCells, TArray<FBlocklandPlacedBlock>& OutPlacedCells) const;
    int32 GetSeed() const { return WorldSeed; }
    FVector GetBlockCenter(const FIntVector& Cell) const;

private:
    struct FChunkData
    {
        TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
    };

    void LoadMaterials();
    void UpdateChunks();
    void CreateChunk(const FIntPoint& ChunkCoordinate);
    void DestroyChunk(const FIntPoint& ChunkCoordinate);
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateComponent(const FIntPoint& ChunkCoordinate, EBlocklandBlock Type);
    void AddBlock(const FIntPoint& ChunkCoordinate, const FIntVector& Cell, EBlocklandBlock Type);
    bool IsCellOccupied(const FIntVector& Cell) const;
    FIntPoint ChunkForCell(const FIntVector& Cell) const;

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UDirectionalLightComponent> SunLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UExponentialHeightFogComponent> DistanceFog;

    UPROPERTY(Transient)
    TObjectPtr<ABlocklandCharacter> Player;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> BlockMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> LiveComponents;

    TMap<FIntPoint, FChunkData> Chunks;
    TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<FIntVector>> ComponentCells;
    TMap<UHierarchicalInstancedStaticMeshComponent*, EBlocklandBlock> ComponentTypes;
    TSet<FIntVector> ActiveCells;
    TSet<FIntVector> RemovedCells;
    TMap<FIntVector, EBlocklandBlock> PlacedCells;

    int32 WorldSeed = 14827;
    float DayTime = 0.28f;
    FIntPoint CurrentChunk = FIntPoint(0, 0);
    static constexpr int32 ChunkSize = 16;
    static constexpr int32 ChunkRadius = 2;
    static constexpr float BlockSize = 100.0f;
};
