#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BlocklandGameMode.generated.h"

UCLASS()
class BLOCKLAND_API ABlocklandGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABlocklandGameMode();

    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
