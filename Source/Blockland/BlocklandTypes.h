#pragma once

#include "CoreMinimal.h"
#include "BlocklandTypes.generated.h"

UENUM(BlueprintType)
enum class EBlocklandBlock : uint8
{
    Grass,
    Dirt,
    Stone,
    Wood,
    Leaves,
    Sand,
    Crystal,
    Water,
    Count
};

static constexpr int32 BlocklandBlockCount = static_cast<int32>(EBlocklandBlock::Count);

USTRUCT(BlueprintType)
struct FBlocklandPlacedBlock
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FIntVector Cell = FIntVector::ZeroValue;

    UPROPERTY(SaveGame)
    EBlocklandBlock Type = EBlocklandBlock::Grass;
};
