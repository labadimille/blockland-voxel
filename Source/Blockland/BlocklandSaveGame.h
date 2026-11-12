#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BlocklandTypes.h"
#include "BlocklandSaveGame.generated.h"

UCLASS()
class BLOCKLAND_API UBlocklandSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 WorldSeed = 14827;

    UPROPERTY(SaveGame)
    FVector PlayerLocation = FVector::ZeroVector;

    UPROPERTY(SaveGame)
    FRotator PlayerRotation = FRotator::ZeroRotator;

    UPROPERTY(SaveGame)
    int32 Health = 100;

    UPROPERTY(SaveGame)
    int32 SelectedSlot = 0;

    UPROPERTY(SaveGame)
    TArray<int32> Inventory;

    UPROPERTY(SaveGame)
    TArray<FIntVector> RemovedBlocks;

    UPROPERTY(SaveGame)
    TArray<FBlocklandPlacedBlock> PlacedBlocks;
};
