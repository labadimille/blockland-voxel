#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlocklandHUD.generated.h"

UCLASS()
class BLOCKLAND_API ABlocklandHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawLabel(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color);
    void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Color);
};
