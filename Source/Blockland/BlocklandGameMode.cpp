#include "BlocklandGameMode.h"
#include "BlocklandCharacter.h"
#include "BlocklandHUD.h"

ABlocklandGameMode::ABlocklandGameMode()
{
    DefaultPawnClass = ABlocklandCharacter::StaticClass();
    HUDClass = ABlocklandHUD::StaticClass();
}

AActor* ABlocklandGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AActor* Start = Super::ChoosePlayerStart_Implementation(Player);
    return Start ? Start : this;
}
