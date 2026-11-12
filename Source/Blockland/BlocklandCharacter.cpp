#include "BlocklandCharacter.h"
#include "BlocklandCreature.h"
#include "BlocklandSaveGame.h"
#include "BlocklandVoxelWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const TCHAR* SaveSlotName = TEXT("Blockland_Save");
    const EBlocklandBlock HotbarBlocks[ABlocklandCharacter::HotbarSlotCount] = {
        EBlocklandBlock::Grass, EBlocklandBlock::Stone, EBlocklandBlock::Wood,
        EBlocklandBlock::Sand, EBlocklandBlock::Dirt
    };

    const TCHAR* BlockNames[BlocklandBlockCount] = {
        TEXT("Erba"), TEXT("Terra"), TEXT("Pietra"), TEXT("Legno"),
        TEXT("Foglie"), TEXT("Sabbia"), TEXT("Cristallo"), TEXT("Acqua")
    };
}

ABlocklandCharacter::ABlocklandCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = 520.0f;
    GetCharacterMovement()->JumpZVelocity = 470.0f;
    GetCharacterMovement()->AirControl = 0.48f;
    GetCapsuleComponent()->InitCapsuleSize(40.0f, 96.0f);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->FieldOfView = 90.0f;

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    WeaponRoot->SetupAttachment(FirstPersonCamera);
    WeaponRoot->SetRelativeLocation(FVector(58.0f, 24.0f, -27.0f));
    WeaponRoot->SetRelativeRotation(FRotator(-7.0f, -11.0f, -8.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cube = CubeAsset.Succeeded() ? CubeAsset.Object : nullptr;

    WeaponHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponHandle"));
    WeaponHandle->SetupAttachment(WeaponRoot);
    WeaponHandle->SetStaticMesh(Cube);
    WeaponHandle->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
    WeaponHandle->SetRelativeScale3D(FVector(0.075f, 0.075f, 0.28f));
    WeaponHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponGuard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponGuard"));
    WeaponGuard->SetupAttachment(WeaponRoot);
    WeaponGuard->SetStaticMesh(Cube);
    WeaponGuard->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
    WeaponGuard->SetRelativeScale3D(FVector(0.25f, 0.08f, 0.07f));
    WeaponGuard->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBlade"));
    WeaponBlade->SetupAttachment(WeaponRoot);
    WeaponBlade->SetStaticMesh(Cube);
    WeaponBlade->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
    WeaponBlade->SetRelativeScale3D(FVector(0.075f, 0.075f, 0.47f));
    WeaponBlade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial)
    {
        UMaterialInstanceDynamic* HandleMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        HandleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.12f, 0.07f));
        WeaponHandle->SetMaterial(0, HandleMaterial);

        UMaterialInstanceDynamic* GuardMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        GuardMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.76f, 0.48f, 0.15f));
        WeaponGuard->SetMaterial(0, GuardMaterial);

        UMaterialInstanceDynamic* BladeMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        BladeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.62f, 0.83f, 0.87f));
        WeaponBlade->SetMaterial(0, BladeMaterial);
    }

    Inventory.SetNumZeroed(BlocklandBlockCount);
}

void ABlocklandCharacter::BeginPlay()
{
    Super::BeginPlay();
    SetDefaultInventory();
    const bool bLoaded = LoadProgress();

    const FVector SavedLocation = GetActorLocation();
    if (!bLoaded || SavedLocation.IsNearlyZero())
    {
        SetActorLocation(FVector(50.0f, 50.0f, 600.0f));
    }

    VoxelWorld = GetWorld()->SpawnActor<ABlocklandVoxelWorld>();
    if (VoxelWorld)
    {
        VoxelWorld->Initialize(this, WorldSeed, RemovedCells, PlacedCells);
        const int32 CellX = FMath::FloorToInt(GetActorLocation().X / BlockSize);
        const int32 CellY = FMath::FloorToInt(GetActorLocation().Y / BlockSize);
        const float Ground = (VoxelWorld->GetSurfaceHeight(CellX, CellY) + 1) * BlockSize;
        FVector Location = GetActorLocation();
        Location.Z = bLoaded ? FMath::Max(Location.Z, Ground + 96.0f) : Ground + 96.0f;
        SetActorLocation(Location);
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    SpawnCreatures();
    ShowMessage(TEXT("Raccogli, costruisci e difendi il tuo rifugio."));
}

void ABlocklandCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
    DodgeCooldown = FMath::Max(0.0f, DodgeCooldown - DeltaSeconds);
    InvulnerabilityTime = FMath::Max(0.0f, InvulnerabilityTime - DeltaSeconds);
    StatusMessageTime = FMath::Max(0.0f, StatusMessageTime - DeltaSeconds);
    SaveTimer += DeltaSeconds;
    if (SaveTimer >= 15.0f)
    {
        SaveTimer = 0.0f;
        SaveProgress();
    }

    if (SwingTime > 0.0f)
    {
        SwingTime = FMath::Max(0.0f, SwingTime - DeltaSeconds);
        const float Alpha = 1.0f - SwingTime / 0.28f;
        WeaponRoot->SetRelativeRotation(FRotator(
            -7.0f - FMath::Sin(Alpha * PI) * 55.0f,
            -11.0f,
            -8.0f + FMath::Sin(Alpha * PI) * 75.0f));
    }
    else
    {
        WeaponRoot->SetRelativeRotation(FMath::RInterpTo(
            WeaponRoot->GetRelativeRotation(),
            FRotator(-7.0f, -11.0f, -8.0f),
            DeltaSeconds,
            12.0f));
    }
}

void ABlocklandCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlocklandCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlocklandCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlocklandCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlocklandCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ABlocklandCharacter::StartJump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ABlocklandCharacter::StopJump);
    PlayerInputComponent->BindAction(TEXT("Mine"), IE_Pressed, this, &ABlocklandCharacter::Mine);
    PlayerInputComponent->BindAction(TEXT("Build"), IE_Pressed, this, &ABlocklandCharacter::Build);
    PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &ABlocklandCharacter::Attack);
    PlayerInputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &ABlocklandCharacter::ToggleInventory);
    PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &ABlocklandCharacter::Dodge);
    PlayerInputComponent->BindAction(TEXT("NextSlot"), IE_Pressed, this, &ABlocklandCharacter::NextSlotAction);
    PlayerInputComponent->BindAction(TEXT("PreviousSlot"), IE_Pressed, this, &ABlocklandCharacter::PreviousSlotAction);
    PlayerInputComponent->BindAction(TEXT("Slot1"), IE_Pressed, this, &ABlocklandCharacter::SelectSlot1);
    PlayerInputComponent->BindAction(TEXT("Slot2"), IE_Pressed, this, &ABlocklandCharacter::SelectSlot2);
    PlayerInputComponent->BindAction(TEXT("Slot3"), IE_Pressed, this, &ABlocklandCharacter::SelectSlot3);
    PlayerInputComponent->BindAction(TEXT("Slot4"), IE_Pressed, this, &ABlocklandCharacter::SelectSlot4);
    PlayerInputComponent->BindAction(TEXT("Slot5"), IE_Pressed, this, &ABlocklandCharacter::SelectSlot5);
    PlayerInputComponent->BindAction(TEXT("NewWorld"), IE_Pressed, this, &ABlocklandCharacter::CreateNewWorld);
}

void ABlocklandCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SaveProgress();
    Super::EndPlay(EndPlayReason);
}

void ABlocklandCharacter::MoveForward(float Value)
{
    if (!bInventoryOpen && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void ABlocklandCharacter::StartJump()
{
    if (!bInventoryOpen)
    {
        Jump();
    }
}

void ABlocklandCharacter::StopJump()
{
    StopJumping();
}

void ABlocklandCharacter::MoveRight(float Value)
{
    if (!bInventoryOpen && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void ABlocklandCharacter::Turn(float Value)
{
    if (!bInventoryOpen)
    {
        AddControllerYawInput(Value);
    }
}

void ABlocklandCharacter::LookUp(float Value)
{
    if (!bInventoryOpen)
    {
        AddControllerPitchInput(Value);
    }
}

bool ABlocklandCharacter::TraceAim(float Distance, FHitResult& OutHit) const
{
    if (!GetWorld() || !FirstPersonCamera)
    {
        return false;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BlocklandAim), true, this);
    Params.AddIgnoredActor(this);
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * Distance;
    return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

void ABlocklandCharacter::Mine()
{
    if (bInventoryOpen || !VoxelWorld)
    {
        return;
    }
    FHitResult Hit;
    EBlocklandBlock Type;
    if (TraceAim(650.0f, Hit) && VoxelWorld->RemoveBlock(Hit, Type))
    {
        AddBlockToInventory(Type, 1);
        SwingTime = 0.28f;
        ShowMessage(FString::Printf(TEXT("Raccolto: %s"), BlockNames[static_cast<int32>(Type)]));
    }
}

void ABlocklandCharacter::Build()
{
    if (bInventoryOpen || !VoxelWorld)
    {
        return;
    }
    const EBlocklandBlock Type = GetSelectedBlock();
    if (GetInventoryCount(Type) <= 0)
    {
        ShowMessage(TEXT("Non hai quel blocco nello zaino."));
        return;
    }
    FHitResult Hit;
    if (TraceAim(650.0f, Hit) && VoxelWorld->PlaceBlock(Hit, Type, GetActorLocation()))
    {
        Inventory[static_cast<int32>(Type)]--;
        ShowMessage(FString::Printf(TEXT("Posato: %s"), BlockNames[static_cast<int32>(Type)]));
        SaveProgress();
    }
}

void ABlocklandCharacter::Attack()
{
    if (bInventoryOpen || AttackCooldown > 0.0f)
    {
        return;
    }
    AttackCooldown = 0.42f;
    SwingTime = 0.28f;
    FHitResult Hit;
    if (TraceAim(420.0f, Hit))
    {
        if (ABlocklandCreature* Creature = Cast<ABlocklandCreature>(Hit.GetActor()))
        {
            Creature->ReceiveHit(35.0f);
            ShowMessage(TEXT("Colpo a segno."));
            return;
        }
    }
    ShowMessage(TEXT("Nessuna creatura a portata."));
}

void ABlocklandCharacter::ToggleInventory()
{
    bInventoryOpen = !bInventoryOpen;
    SetInventoryVisibility(bInventoryOpen);
}

void ABlocklandCharacter::SetInventoryVisibility(bool bVisible)
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = bVisible;
        if (bVisible)
        {
            PC->SetInputMode(FInputModeGameAndUI());
        }
        else
        {
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}

void ABlocklandCharacter::Dodge()
{
    if (bInventoryOpen || DodgeCooldown > 0.0f)
    {
        return;
    }
    DodgeCooldown = 1.1f;
    LaunchCharacter(GetActorForwardVector() * 760.0f + FVector(0.0f, 0.0f, 100.0f), true, true);
}

void ABlocklandCharacter::SelectSlot(int32 Slot)
{
    SelectedSlot = FMath::Clamp(Slot, 0, HotbarSlotCount - 1);
}

void ABlocklandCharacter::NextSlot(int32 Direction)
{
    SelectedSlot = (SelectedSlot + Direction + HotbarSlotCount) % HotbarSlotCount;
}

EBlocklandBlock ABlocklandCharacter::GetSelectedBlock() const
{
    return HotbarBlocks[FMath::Clamp(SelectedSlot, 0, HotbarSlotCount - 1)];
}

int32 ABlocklandCharacter::GetInventoryCount(EBlocklandBlock Type) const
{
    const int32 Index = static_cast<int32>(Type);
    return Inventory.IsValidIndex(Index) ? Inventory[Index] : 0;
}

void ABlocklandCharacter::AddBlockToInventory(EBlocklandBlock Type, int32 Amount)
{
    const int32 Index = static_cast<int32>(Type);
    if (Inventory.IsValidIndex(Index) && Amount > 0)
    {
        Inventory[Index] = FMath::Min(999, Inventory[Index] + Amount);
        SaveProgress();
    }
}

void ABlocklandCharacter::ApplyDamage(int32 Amount)
{
    if (InvulnerabilityTime > 0.0f || Amount <= 0)
    {
        return;
    }
    Health = FMath::Max(0, Health - Amount);
    InvulnerabilityTime = 0.85f;
    if (Health <= 0)
    {
        Health = MaxHealth;
        const int32 CellX = 0;
        const int32 CellY = 0;
        const float Ground = VoxelWorld
            ? (VoxelWorld->GetSurfaceHeight(CellX, CellY) + 1) * BlockSize
            : 600.0f;
        SetActorLocation(FVector(50.0f, 50.0f, Ground + 96.0f));
        ShowMessage(TEXT("Sei tornato al campo base."));
    }
    else
    {
        ShowMessage(FString::Printf(TEXT("Colpito! Salute: %d"), Health));
    }
    SaveProgress();
}

void ABlocklandCharacter::ShowMessage(const FString& Message)
{
    StatusMessage = Message;
    StatusMessageTime = 2.2f;
}

void ABlocklandCharacter::SetDefaultInventory()
{
    Inventory.SetNumZeroed(BlocklandBlockCount);
    Inventory[static_cast<int32>(EBlocklandBlock::Grass)] = 16;
    Inventory[static_cast<int32>(EBlocklandBlock::Dirt)] = 12;
    Inventory[static_cast<int32>(EBlocklandBlock::Stone)] = 12;
    Inventory[static_cast<int32>(EBlocklandBlock::Wood)] = 8;
    Inventory[static_cast<int32>(EBlocklandBlock::Sand)] = 8;
}

bool ABlocklandCharacter::LoadProgress()
{
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        return false;
    }
    UBlocklandSaveGame* Save = Cast<UBlocklandSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save)
    {
        UE_LOG(LogTemp, Warning, TEXT("Blockland: il salvataggio esistente non e' leggibile."));
        return false;
    }

    WorldSeed = Save->WorldSeed;
    Health = FMath::Clamp(Save->Health, 1, MaxHealth);
    SelectedSlot = FMath::Clamp(Save->SelectedSlot, 0, HotbarSlotCount - 1);
    if (Save->Inventory.Num() == BlocklandBlockCount)
    {
        Inventory = Save->Inventory;
    }
    RemovedCells = Save->RemovedBlocks;
    PlacedCells = Save->PlacedBlocks;
    SetActorLocation(Save->PlayerLocation);
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetControlRotation(Save->PlayerRotation);
    }
    return true;
}

void ABlocklandCharacter::SaveProgress()
{
    if (!GetWorld())
    {
        return;
    }
    UBlocklandSaveGame* Save = Cast<UBlocklandSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UBlocklandSaveGame::StaticClass()));
    if (!Save)
    {
        UE_LOG(LogTemp, Error, TEXT("Blockland: impossibile creare il salvataggio."));
        return;
    }

    Save->WorldSeed = WorldSeed;
    Save->PlayerLocation = GetActorLocation();
    Save->PlayerRotation = GetControlRotation();
    Save->Health = Health;
    Save->SelectedSlot = SelectedSlot;
    Save->Inventory = Inventory;
    if (VoxelWorld)
    {
        VoxelWorld->ExportState(Save->RemovedBlocks, Save->PlacedBlocks);
    }
    if (!UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Blockland: impossibile salvare la partita."));
    }
}

void ABlocklandCharacter::SpawnCreatures()
{
    if (!GetWorld())
    {
        return;
    }

    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FVector SpawnOffsets[] = {
        Forward * 1050.0f + Right * 350.0f,
        Forward * 1550.0f - Right * 600.0f
    };
    for (const FVector& Offset : SpawnOffsets)
    {
        FVector Location = GetActorLocation() + Offset;
        if (VoxelWorld)
        {
            const int32 CellX = FMath::FloorToInt(Location.X / BlockSize);
            const int32 CellY = FMath::FloorToInt(Location.Y / BlockSize);
            Location.Z = (VoxelWorld->GetSurfaceHeight(CellX, CellY) + 1) * BlockSize;
        }
        GetWorld()->SpawnActor<ABlocklandCreature>(
            ABlocklandCreature::StaticClass(),
            Location,
            FRotationMatrix::MakeFromX(GetActorLocation() - Location).Rotator());
    }
}

void ABlocklandCharacter::CreateNewWorld()
{
    WorldSeed = FMath::RandRange(1, MAX_int32);
    Health = MaxHealth;
    SelectedSlot = 0;
    SetDefaultInventory();
    RemovedCells.Reset();
    PlacedCells.Reset();
    if (VoxelWorld)
    {
        VoxelWorld->Destroy();
    }
    for (TActorIterator<ABlocklandCreature> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    SetActorLocation(FVector(50.0f, 50.0f, 600.0f));
    VoxelWorld = GetWorld()->SpawnActor<ABlocklandVoxelWorld>();
    if (VoxelWorld)
    {
        VoxelWorld->Initialize(this, WorldSeed, RemovedCells, PlacedCells);
        const float Ground = (VoxelWorld->GetSurfaceHeight(0, 0) + 1) * BlockSize;
        SetActorLocation(FVector(50.0f, 50.0f, Ground + 96.0f));
    }
    SpawnCreatures();
    ShowMessage(TEXT("Un nuovo mondo ti aspetta."));
    SaveProgress();
}
