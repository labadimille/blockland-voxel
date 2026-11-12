#include "BlocklandCreature.h"
#include "BlocklandCharacter.h"
#include "BlocklandVoxelWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABlocklandCreature::ABlocklandCreature()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cube = CubeAsset.Succeeded() ? CubeAsset.Object : nullptr;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(SceneRoot);
    Body->SetStaticMesh(Cube);
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, 56.0f));
    Body->SetRelativeScale3D(FVector(0.88f, 0.76f, 0.78f));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body->SetGenerateOverlapEvents(false);
    Body->SetCanEverAffectNavigation(false);

    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
    Head->SetupAttachment(SceneRoot);
    Head->SetStaticMesh(Cube);
    Head->SetRelativeLocation(FVector(25.0f, 0.0f, 105.0f));
    Head->SetRelativeScale3D(FVector(0.58f, 0.58f, 0.58f));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    EyeLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeLeft"));
    EyeLeft->SetupAttachment(Head);
    EyeLeft->SetStaticMesh(Cube);
    EyeLeft->SetRelativeLocation(FVector(0.88f, -0.22f, 0.08f));
    EyeLeft->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.17f));
    EyeLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    EyeRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeRight"));
    EyeRight->SetupAttachment(Head);
    EyeRight->SetStaticMesh(Cube);
    EyeRight->SetRelativeLocation(FVector(0.88f, 0.22f, 0.08f));
    EyeRight->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.17f));
    EyeRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UMaterialInterface* BodyBase = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Game/Blockland/Materials/M_Creature.M_Creature"));
    UMaterialInterface* EyeBase = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Game/Blockland/Materials/M_Crystal.M_Crystal"));
    bUsesTintMaterial = BodyBase != nullptr;
    if (!BodyBase)
    {
        BodyBase = LoadObject<UMaterialInterface>(
            nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }
    if (!EyeBase)
    {
        EyeBase = BodyBase;
    }
    if (BodyBase)
    {
        BodyMaterial = UMaterialInstanceDynamic::Create(BodyBase, this);
        BodyMaterial->SetVectorParameterValue(
            bUsesTintMaterial ? TEXT("Tint") : TEXT("Color"),
            FLinearColor(0.78f, 0.93f, 0.69f));
        EyeMaterial = UMaterialInstanceDynamic::Create(EyeBase, this);
        EyeMaterial->SetVectorParameterValue(
            bUsesTintMaterial ? TEXT("Tint") : TEXT("Color"),
            FLinearColor(1.0f, 0.76f, 0.34f));
        Body->SetMaterial(0, BodyMaterial);
        Head->SetMaterial(0, BodyMaterial);
        EyeLeft->SetMaterial(0, EyeMaterial);
        EyeRight->SetMaterial(0, EyeMaterial);
    }
}

void ABlocklandCreature::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
    HitFlash = FMath::Max(0.0f, HitFlash - DeltaSeconds);
    if (BodyMaterial)
    {
        BodyMaterial->SetVectorParameterValue(
            bUsesTintMaterial ? TEXT("Tint") : TEXT("Color"),
            HitFlash > 0.0f ? FLinearColor::White : FLinearColor(0.78f, 0.93f, 0.69f));
    }

    ABlocklandCharacter* Player = Cast<ABlocklandCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player)
    {
        return;
    }

    FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    ToPlayer.Z = 0.0f;
    const float Distance = ToPlayer.Size();
    if (Distance > 135.0f)
    {
        const FVector Direction = ToPlayer.GetSafeNormal();
        const float Travel = FMath::Min(205.0f * DeltaSeconds, Distance - 135.0f);
        FVector NewLocation = GetActorLocation() + Direction * Travel;
        if (ABlocklandVoxelWorld* VoxelWorld = Player->GetVoxelWorld())
        {
            const int32 CellX = FMath::FloorToInt(NewLocation.X / 100.0f);
            const int32 CellY = FMath::FloorToInt(NewLocation.Y / 100.0f);
            NewLocation.Z = (VoxelWorld->GetSurfaceHeight(CellX, CellY) + 1) * 100.0f;
        }
        SetActorLocation(NewLocation, false);
        SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    }
    else if (AttackCooldown <= 0.0f)
    {
        AttackCooldown = 1.65f;
        Player->ApplyDamage(14);
    }
}

void ABlocklandCreature::ReceiveHit(float Damage)
{
    Health -= Damage;
    HitFlash = 0.18f;
    if (Health <= 0.0f)
    {
        if (ABlocklandCharacter* Player = Cast<ABlocklandCharacter>(
            UGameplayStatics::GetPlayerCharacter(this, 0)))
        {
            Player->AddBlockToInventory(EBlocklandBlock::Crystal, 1);
            Player->ShowMessage(TEXT("La creatura ha lasciato un cristallo."));
        }
        Destroy();
    }
}
