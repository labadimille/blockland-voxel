#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlocklandTypes.h"
#include "BlocklandCharacter.generated.h"

class ABlocklandVoxelWorld;
class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class BLOCKLAND_API ABlocklandCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABlocklandCharacter();
    static constexpr int32 HotbarSlotCount = 5;

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    int32 GetHealth() const { return Health; }
    int32 GetMaxHealth() const { return MaxHealth; }
    int32 GetSelectedSlot() const { return SelectedSlot; }
    EBlocklandBlock GetSelectedBlock() const;
    int32 GetInventoryCount(EBlocklandBlock Type) const;
    bool IsInventoryOpen() const { return bInventoryOpen; }
    ABlocklandVoxelWorld* GetVoxelWorld() const { return VoxelWorld; }
    const FString& GetStatusMessage() const { return StatusMessage; }
    float GetStatusMessageTime() const { return StatusMessageTime; }

    void AddBlockToInventory(EBlocklandBlock Type, int32 Amount);
    void ApplyDamage(int32 Amount);
    void ShowMessage(const FString& Message);

private:
    virtual void BeginPlay() override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartJump();
    void StopJump();
    void Mine();
    void Build();
    void Attack();
    void ToggleInventory();
    void Dodge();
    void SelectSlot(int32 Slot);
    void NextSlot(int32 Direction);
    void CreateNewWorld();
    void SpawnCreatures();
    void SetDefaultInventory();
    void SaveProgress();
    bool LoadProgress();
    bool TraceAim(float Distance, FHitResult& OutHit) const;
    void SetInventoryVisibility(bool bVisible);

    void SelectSlot1() { SelectSlot(0); }
    void SelectSlot2() { SelectSlot(1); }
    void SelectSlot3() { SelectSlot(2); }
    void SelectSlot4() { SelectSlot(3); }
    void SelectSlot5() { SelectSlot(4); }
    void NextSlotAction() { NextSlot(1); }
    void PreviousSlotAction() { NextSlot(-1); }

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> WeaponRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> WeaponHandle;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> WeaponGuard;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> WeaponBlade;

    UPROPERTY(Transient)
    TObjectPtr<ABlocklandVoxelWorld> VoxelWorld;

    UPROPERTY(Transient)
    TArray<int32> Inventory;

    int32 Health = 100;
    int32 SelectedSlot = 0;
    int32 WorldSeed = 14827;
    bool bInventoryOpen = false;
    float SwingTime = 0.0f;
    float AttackCooldown = 0.0f;
    float DodgeCooldown = 0.0f;
    float InvulnerabilityTime = 0.0f;
    float StatusMessageTime = 0.0f;
    float SaveTimer = 0.0f;
    FString StatusMessage;
    TArray<FIntVector> RemovedCells;
    TArray<FBlocklandPlacedBlock> PlacedCells;

    static constexpr int32 MaxHealth = 100;
    static constexpr float BlockSize = 100.0f;
};
