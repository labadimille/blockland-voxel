#include "BlocklandVoxelWorld.h"
#include "BlocklandCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const TCHAR* MaterialNames[BlocklandBlockCount] = {
        TEXT("Grass"), TEXT("Dirt"), TEXT("Stone"), TEXT("Wood"),
        TEXT("Leaves"), TEXT("Sand"), TEXT("Crystal"), TEXT("Water")
    };

    const FLinearColor FallbackColors[BlocklandBlockCount] = {
        FLinearColor(0.20f, 0.48f, 0.20f),
        FLinearColor(0.43f, 0.25f, 0.15f),
        FLinearColor(0.36f, 0.43f, 0.49f),
        FLinearColor(0.48f, 0.29f, 0.14f),
        FLinearColor(0.12f, 0.37f, 0.19f),
        FLinearColor(0.69f, 0.57f, 0.34f),
        FLinearColor(0.08f, 0.62f, 0.69f),
        FLinearColor(0.10f, 0.34f, 0.59f)
    };

    uint32 CellHash(int32 X, int32 Y, int32 Seed)
    {
        uint32 Value = static_cast<uint32>(X) * 0x8DA6B343u;
        Value ^= static_cast<uint32>(Y) * 0xD8163841u;
        Value ^= static_cast<uint32>(Seed) * 0xCB1AB31Fu;
        Value ^= Value >> 13;
        Value *= 0x85EBCA6Bu;
        Value ^= Value >> 16;
        return Value;
    }

    int32 FloorDiv(int32 Value, int32 Divisor)
    {
        return FMath::FloorToInt(static_cast<float>(Value) / static_cast<float>(Divisor));
    }
}

ABlocklandVoxelWorld::ABlocklandVoxelWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SceneRoot->SetMobility(EComponentMobility::Movable);
    SetRootComponent(SceneRoot);

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    SunLight->SetupAttachment(SceneRoot);
    SunLight->SetMobility(EComponentMobility::Movable);
    SunLight->SetRelativeRotation(FRotator(-42.0f, -32.0f, 0.0f));
    SunLight->SetIntensity(5.5f);
    SunLight->SetLightColor(FLinearColor(1.0f, 0.88f, 0.70f));
    SunLight->SetAtmosphereSunLight(true);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetSourceType(SLS_CapturedScene);
    SkyLight->SetIntensity(0.75f);

    DistanceFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("DistanceFog"));
    DistanceFog->SetupAttachment(SceneRoot);
    DistanceFog->FogDensity = 0.0035f;
    DistanceFog->FogHeightFalloff = 0.22f;
    DistanceFog->FogMaxOpacity = 0.52f;
    DistanceFog->FogInscatteringColor = FLinearColor(0.25f, 0.43f, 0.52f);
    DistanceFog->SetRelativeLocation(FVector(0.0f, 0.0f, 280.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
    }
}

void ABlocklandVoxelWorld::Initialize(
    ABlocklandCharacter* InPlayer,
    int32 InSeed,
    const TArray<FIntVector>& InRemovedCells,
    const TArray<FBlocklandPlacedBlock>& InPlacedCells)
{
    Player = InPlayer;
    WorldSeed = InSeed;
    RemovedCells.Reset();
    for (const FIntVector& Cell : InRemovedCells)
    {
        RemovedCells.Add(Cell);
    }
    PlacedCells.Reset();
    for (const FBlocklandPlacedBlock& Block : InPlacedCells)
    {
        PlacedCells.Add(Block.Cell, Block.Type);
    }

    LoadMaterials();
    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Blockland: il cubo base di Unreal non e' disponibile."));
        return;
    }
    UpdateChunks();
}

void ABlocklandVoxelWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    DayTime = FMath::Fmod(DayTime + DeltaSeconds / 480.0f, 1.0f);
    const float SunElevation = FMath::Sin(DayTime * 2.0f * PI - HALF_PI);
    const float Daylight = FMath::Clamp((SunElevation + 0.12f) / 0.66f, 0.0f, 1.0f);
    if (SunLight)
    {
        SunLight->SetRelativeRotation(FRotator(-SunElevation * 68.0f, -32.0f, 0.0f));
        SunLight->SetIntensity(FMath::Lerp(0.12f, 5.5f, Daylight));
        SunLight->SetLightColor(FMath::Lerp(
            FLinearColor(1.0f, 0.48f, 0.26f),
            FLinearColor(1.0f, 0.88f, 0.70f),
            Daylight));
    }
    if (SkyLight)
    {
        SkyLight->SetIntensity(FMath::Lerp(0.32f, 0.82f, Daylight));
    }

    if (Player)
    {
        const FVector Location = Player->GetActorLocation();
        const FIntPoint PlayerChunk(
            FloorDiv(FMath::FloorToInt(Location.X / BlockSize), ChunkSize),
            FloorDiv(FMath::FloorToInt(Location.Y / BlockSize), ChunkSize));
        if (PlayerChunk != CurrentChunk)
        {
            UpdateChunks();
        }
    }
}

int32 ABlocklandVoxelWorld::GetSurfaceHeight(int32 X, int32 Y) const
{
    const float SeedOffsetX = static_cast<float>(WorldSeed % 17011);
    const float SeedOffsetY = static_cast<float>(WorldSeed % 29009);
    const float Broad = FMath::PerlinNoise2D(FVector2D((X + SeedOffsetX) * 0.043f, (Y + SeedOffsetY) * 0.043f));
    const float Detail = FMath::PerlinNoise2D(FVector2D((X - SeedOffsetY) * 0.12f, (Y + SeedOffsetX) * 0.12f));
    return FMath::Clamp(FMath::RoundToInt(4.5f + Broad * 3.2f + Detail * 0.8f), 1, 9);
}

FVector ABlocklandVoxelWorld::GetBlockCenter(const FIntVector& Cell) const
{
    return FVector(
        (Cell.X + 0.5f) * BlockSize,
        (Cell.Y + 0.5f) * BlockSize,
        (Cell.Z + 0.5f) * BlockSize);
}

void ABlocklandVoxelWorld::LoadMaterials()
{
    BlockMaterials.SetNum(BlocklandBlockCount);
    UMaterialInterface* Fallback = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    for (int32 Index = 0; Index < BlocklandBlockCount; ++Index)
    {
        const FString AssetPath = FString::Printf(
            TEXT("/Game/Blockland/Materials/M_%s.M_%s"),
            MaterialNames[Index], MaterialNames[Index]);
        UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
        if (!Loaded && Fallback)
        {
            UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Fallback, this);
            Dynamic->SetVectorParameterValue(TEXT("Color"), FallbackColors[Index]);
            Loaded = Dynamic;
        }
        BlockMaterials[Index] = Loaded;
    }
}

void ABlocklandVoxelWorld::UpdateChunks()
{
    if (!Player)
    {
        return;
    }

    const FVector Location = Player->GetActorLocation();
    const FIntPoint Center(
        FloorDiv(FMath::FloorToInt(Location.X / BlockSize), ChunkSize),
        FloorDiv(FMath::FloorToInt(Location.Y / BlockSize), ChunkSize));
    if (Center == CurrentChunk && Chunks.Num() > 0)
    {
        return;
    }
    CurrentChunk = Center;

    TSet<FIntPoint> Desired;
    for (int32 X = -ChunkRadius; X <= ChunkRadius; ++X)
    {
        for (int32 Y = -ChunkRadius; Y <= ChunkRadius; ++Y)
        {
            const FIntPoint Coordinate(Center.X + X, Center.Y + Y);
            Desired.Add(Coordinate);
            if (!Chunks.Contains(Coordinate))
            {
                CreateChunk(Coordinate);
            }
        }
    }
    if (SkyLight)
    {
        SkyLight->RecaptureSky();
    }

    TArray<FIntPoint> Existing;
    Chunks.GetKeys(Existing);
    for (const FIntPoint& Coordinate : Existing)
    {
        if (!Desired.Contains(Coordinate))
        {
            DestroyChunk(Coordinate);
        }
    }
}

void ABlocklandVoxelWorld::CreateChunk(const FIntPoint& ChunkCoordinate)
{
    TArray<TArray<FIntVector>> Batches;
    Batches.SetNum(BlocklandBlockCount);
    TSet<FIntVector> ChunkCells;
    const int32 MinX = ChunkCoordinate.X * ChunkSize;
    const int32 MinY = ChunkCoordinate.Y * ChunkSize;
    const int32 SeaLevel = 3;

    auto AddCell = [&](const FIntVector& Cell, EBlocklandBlock Type)
    {
        if (RemovedCells.Contains(Cell) || PlacedCells.Contains(Cell) || ChunkCells.Contains(Cell))
        {
            return;
        }
        ChunkCells.Add(Cell);
        Batches[static_cast<int32>(Type)].Add(Cell);
    };

    for (int32 X = MinX; X < MinX + ChunkSize; ++X)
    {
        for (int32 Y = MinY; Y < MinY + ChunkSize; ++Y)
        {
            const int32 Height = GetSurfaceHeight(X, Y);
            const bool bBeach = Height <= SeaLevel;
            for (int32 Z = 0; Z <= Height; ++Z)
            {
                const EBlocklandBlock Type = Z == Height
                    ? (bBeach ? EBlocklandBlock::Sand : EBlocklandBlock::Grass)
                    : (Z >= Height - 2 ? EBlocklandBlock::Dirt : EBlocklandBlock::Stone);
                AddCell(FIntVector(X, Y, Z), Type);
            }

            if (bBeach)
            {
                for (int32 Z = Height + 1; Z <= SeaLevel; ++Z)
                {
                    AddCell(FIntVector(X, Y, Z), EBlocklandBlock::Water);
                }
            }

            const uint32 Hash = CellHash(X, Y, WorldSeed);
            const bool bTreeMargin =
                X >= MinX + 2 && X < MinX + ChunkSize - 2 &&
                Y >= MinY + 2 && Y < MinY + ChunkSize - 2;
            if (!bBeach && bTreeMargin && Hash % 100u < 5u)
            {
                for (int32 Z = Height + 1; Z <= Height + 4; ++Z)
                {
                    AddCell(FIntVector(X, Y, Z), EBlocklandBlock::Wood);
                }
                for (int32 DX = -2; DX <= 2; ++DX)
                {
                    for (int32 DY = -2; DY <= 2; ++DY)
                    {
                        for (int32 DZ = 2; DZ <= 5; ++DZ)
                        {
                            const int32 Radius = FMath::Abs(DX) + FMath::Abs(DY) + FMath::Max(0, DZ - 3);
                            if (Radius <= 5)
                            {
                                AddCell(FIntVector(X + DX, Y + DY, Height + DZ), EBlocklandBlock::Leaves);
                            }
                        }
                    }
                }
            }
            else if (!bBeach && Hash % 149u == 0u)
            {
                const FIntVector CrystalCell(X, Y, FMath::Max(1, Height - 3));
                if (ChunkCells.Contains(CrystalCell))
                {
                    Batches[static_cast<int32>(EBlocklandBlock::Stone)].Remove(CrystalCell);
                    ChunkCells.Remove(CrystalCell);
                    AddCell(CrystalCell, EBlocklandBlock::Crystal);
                }
            }
        }
    }

    for (const TPair<FIntVector, EBlocklandBlock>& Pair : PlacedCells)
    {
        if (ChunkForCell(Pair.Key) == ChunkCoordinate)
        {
            ChunkCells.Add(Pair.Key);
            Batches[static_cast<int32>(Pair.Value)].Add(Pair.Key);
        }
    }

    FChunkData& Chunk = Chunks.Add(ChunkCoordinate);
    Chunk.Components.SetNum(BlocklandBlockCount);
    for (int32 TypeIndex = 0; TypeIndex < BlocklandBlockCount; ++TypeIndex)
    {
        for (const FIntVector& Cell : Batches[TypeIndex])
        {
            AddBlock(ChunkCoordinate, Cell, static_cast<EBlocklandBlock>(TypeIndex));
        }
    }
}

void ABlocklandVoxelWorld::DestroyChunk(const FIntPoint& ChunkCoordinate)
{
    FChunkData* Chunk = Chunks.Find(ChunkCoordinate);
    if (!Chunk)
    {
        return;
    }

    for (UHierarchicalInstancedStaticMeshComponent* Component : Chunk->Components)
    {
        if (!Component)
        {
            continue;
        }
        if (const TArray<FIntVector>* Cells = ComponentCells.Find(Component))
        {
            for (const FIntVector& Cell : *Cells)
            {
                ActiveCells.Remove(Cell);
            }
        }
        ComponentCells.Remove(Component);
        ComponentTypes.Remove(Component);
        LiveComponents.Remove(Component);
        Component->DestroyComponent();
    }
    Chunks.Remove(ChunkCoordinate);
}

UHierarchicalInstancedStaticMeshComponent* ABlocklandVoxelWorld::GetOrCreateComponent(
    const FIntPoint& ChunkCoordinate,
    EBlocklandBlock Type)
{
    FChunkData* Chunk = Chunks.Find(ChunkCoordinate);
    if (!Chunk)
    {
        return nullptr;
    }

    const int32 TypeIndex = static_cast<int32>(Type);
    if (Chunk->Components.IsValidIndex(TypeIndex) && Chunk->Components[TypeIndex])
    {
        return Chunk->Components[TypeIndex];
    }

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(CubeMesh);
    Component->SetCollisionEnabled(Type == EBlocklandBlock::Water
        ? ECollisionEnabled::NoCollision
        : ECollisionEnabled::QueryAndPhysics);
    Component->SetCollisionObjectType(ECC_WorldStatic);
    Component->SetCollisionResponseToAllChannels(ECR_Block);
    Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(
        Type == EBlocklandBlock::Wood ||
        Type == EBlocklandBlock::Leaves ||
        Type == EBlocklandBlock::Crystal);
    if (BlockMaterials.IsValidIndex(TypeIndex) && BlockMaterials[TypeIndex])
    {
        Component->SetMaterial(0, BlockMaterials[TypeIndex]);
    }
    AddInstanceComponent(Component);
    Component->RegisterComponent();

    Chunk->Components[TypeIndex] = Component;
    LiveComponents.Add(Component);
    ComponentCells.Add(Component, TArray<FIntVector>());
    ComponentTypes.Add(Component, Type);
    return Component;
}

void ABlocklandVoxelWorld::AddBlock(
    const FIntPoint& ChunkCoordinate,
    const FIntVector& Cell,
    EBlocklandBlock Type)
{
    UHierarchicalInstancedStaticMeshComponent* Component = GetOrCreateComponent(ChunkCoordinate, Type);
    if (!Component || !CubeMesh)
    {
        return;
    }

    const FVector Center = GetBlockCenter(Cell);
    const FVector Scale = FVector::OneVector;
    Component->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
    ComponentCells.FindChecked(Component).Add(Cell);
    ActiveCells.Add(Cell);
}

bool ABlocklandVoxelWorld::RemoveBlock(const FHitResult& Hit, EBlocklandBlock& OutType)
{
    UHierarchicalInstancedStaticMeshComponent* Component =
        Cast<UHierarchicalInstancedStaticMeshComponent>(Hit.GetComponent());
    TArray<FIntVector>* Cells = ComponentCells.Find(Component);
    if (!Component || !Cells || !Cells->IsValidIndex(Hit.Item))
    {
        return false;
    }

    const EBlocklandBlock* Type = ComponentTypes.Find(Component);
    if (!Type || *Type == EBlocklandBlock::Water)
    {
        return false;
    }

    const FIntVector Cell = (*Cells)[Hit.Item];
    if (!Component->RemoveInstance(Hit.Item))
    {
        return false;
    }

    Cells->RemoveAt(Hit.Item);
    ActiveCells.Remove(Cell);
    PlacedCells.Remove(Cell);
    RemovedCells.Add(Cell);
    OutType = *Type;
    return true;
}

bool ABlocklandVoxelWorld::PlaceBlock(
    const FHitResult& Hit,
    EBlocklandBlock Type,
    const FVector& PlayerLocation)
{
    UHierarchicalInstancedStaticMeshComponent* Component =
        Cast<UHierarchicalInstancedStaticMeshComponent>(Hit.GetComponent());
    const TArray<FIntVector>* Cells = ComponentCells.Find(Component);
    if (!Component || !Cells || !Cells->IsValidIndex(Hit.Item))
    {
        return false;
    }

    const FIntVector Source = (*Cells)[Hit.Item];
    const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
    const FIntVector Offset(
        FMath::RoundToInt(Normal.X),
        FMath::RoundToInt(Normal.Y),
        FMath::RoundToInt(Normal.Z));
    if (Offset == FIntVector::ZeroValue)
    {
        return false;
    }

    const FIntVector Target = Source + Offset;
    if (Target.Z < 0 || IsCellOccupied(Target) ||
        FVector::Dist(GetBlockCenter(Target), PlayerLocation) < 145.0f)
    {
        return false;
    }

    RemovedCells.Add(Target);
    PlacedCells.Add(Target, Type);
    AddBlock(ChunkForCell(Target), Target, Type);
    return true;
}

bool ABlocklandVoxelWorld::IsCellOccupied(const FIntVector& Cell) const
{
    return ActiveCells.Contains(Cell) || PlacedCells.Contains(Cell);
}

FIntPoint ABlocklandVoxelWorld::ChunkForCell(const FIntVector& Cell) const
{
    return FIntPoint(FloorDiv(Cell.X, ChunkSize), FloorDiv(Cell.Y, ChunkSize));
}

void ABlocklandVoxelWorld::ExportState(
    TArray<FIntVector>& OutRemovedCells,
    TArray<FBlocklandPlacedBlock>& OutPlacedCells) const
{
    OutRemovedCells = RemovedCells.Array();
    OutPlacedCells.Reset(PlacedCells.Num());
    for (const TPair<FIntVector, EBlocklandBlock>& Pair : PlacedCells)
    {
        FBlocklandPlacedBlock& Block = OutPlacedCells.AddDefaulted_GetRef();
        Block.Cell = Pair.Key;
        Block.Type = Pair.Value;
    }
}
