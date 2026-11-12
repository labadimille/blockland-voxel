#include "BlocklandHUD.h"
#include "BlocklandCharacter.h"
#include "BlocklandTypes.h"
#include "BlocklandVoxelWorld.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
    const TCHAR* BlockNames[BlocklandBlockCount] = {
        TEXT("Erba"), TEXT("Terra"), TEXT("Pietra"), TEXT("Legno"),
        TEXT("Foglie"), TEXT("Sabbia"), TEXT("Cristallo"), TEXT("Acqua")
    };

    const FLinearColor BlockColors[BlocklandBlockCount] = {
        FLinearColor(0.28f, 0.65f, 0.30f),
        FLinearColor(0.54f, 0.35f, 0.22f),
        FLinearColor(0.48f, 0.58f, 0.65f),
        FLinearColor(0.62f, 0.39f, 0.20f),
        FLinearColor(0.18f, 0.50f, 0.27f),
        FLinearColor(0.78f, 0.67f, 0.43f),
        FLinearColor(0.20f, 0.78f, 0.81f),
        FLinearColor(0.19f, 0.48f, 0.73f)
    };

    constexpr int32 HotbarSlotCount = 5;
    const EBlocklandBlock HotbarBlocks[HotbarSlotCount] = {
        EBlocklandBlock::Grass, EBlocklandBlock::Stone, EBlocklandBlock::Wood,
        EBlocklandBlock::Sand, EBlocklandBlock::Dirt
    };
}

void ABlocklandHUD::DrawLabel(
    const FString& Text,
    float X,
    float Y,
    float Scale,
    const FLinearColor& Color)
{
    if (!Canvas || !GEngine)
    {
        return;
    }
    Canvas->DrawColor = Color.ToFColor(true);
    Canvas->DrawText(GEngine->GetMediumFont(), Text, X, Y, Scale, Scale);
}

void ABlocklandHUD::DrawPanel(
    float X,
    float Y,
    float Width,
    float Height,
    const FLinearColor& Color)
{
    if (Canvas)
    {
        Canvas->DrawRect(Color, X, Y, Width, Height);
    }
}

void ABlocklandHUD::DrawHUD()
{
    Super::DrawHUD();
    ABlocklandCharacter* Player = Cast<ABlocklandCharacter>(GetOwningPawn());
    if (!Player || !Canvas)
    {
        return;
    }

    const float Scale = FMath::Clamp(Canvas->SizeY / 1080.0f, 0.72f, 1.18f);
    const float ScreenW = Canvas->SizeX;
    const float ScreenH = Canvas->SizeY;
    const float Margin = 24.0f * Scale;

    DrawPanel(Margin, Margin, 330.0f * Scale, 83.0f * Scale, FLinearColor(0.035f, 0.07f, 0.09f, 0.82f));
    DrawLabel(TEXT("BLOCKLAND"), Margin + 18.0f * Scale, Margin + 10.0f * Scale, 1.25f * Scale, FLinearColor(0.95f, 0.88f, 0.66f));
    DrawLabel(TEXT("IL MONDO SELVAGGIO"), Margin + 19.0f * Scale, Margin + 46.0f * Scale, 0.68f * Scale, FLinearColor(0.67f, 0.80f, 0.79f));

    const float HealthX = Margin;
    const float HealthY = Margin + 99.0f * Scale;
    const float HealthW = 250.0f * Scale;
    DrawPanel(HealthX, HealthY, HealthW, 35.0f * Scale, FLinearColor(0.025f, 0.045f, 0.055f, 0.78f));
    DrawPanel(HealthX + 5.0f * Scale, HealthY + 6.0f * Scale,
        (HealthW - 10.0f * Scale) * Player->GetHealth() / Player->GetMaxHealth(),
        23.0f * Scale, FLinearColor(0.76f, 0.19f, 0.22f, 0.96f));
    DrawLabel(FString::Printf(TEXT("SALUTE  %d / %d"), Player->GetHealth(), Player->GetMaxHealth()),
        HealthX + 12.0f * Scale, HealthY + 7.0f * Scale, 0.65f * Scale, FLinearColor::White);

    const FVector Location = Player->GetActorLocation() / 100.0f;
    DrawPanel(ScreenW - Margin - 225.0f * Scale, Margin, 225.0f * Scale, 62.0f * Scale,
        FLinearColor(0.035f, 0.07f, 0.09f, 0.82f));
    DrawLabel(FString::Printf(TEXT("X %d   Y %d"), FMath::FloorToInt(Location.X), FMath::FloorToInt(Location.Y)),
        ScreenW - Margin - 208.0f * Scale, Margin + 11.0f * Scale, 0.75f * Scale,
        FLinearColor(0.91f, 0.94f, 0.87f));
    DrawLabel(FString::Printf(TEXT("MONDO %d"), Player->GetVoxelWorld() ? Player->GetVoxelWorld()->GetSeed() : 0),
        ScreenW - Margin - 208.0f * Scale, Margin + 36.0f * Scale, 0.58f * Scale,
        FLinearColor(0.65f, 0.79f, 0.78f));

    const float CenterX = ScreenW * 0.5f;
    const float CenterY = ScreenH * 0.5f;
    DrawPanel(CenterX - 1.0f * Scale, CenterY - 8.0f * Scale, 2.0f * Scale, 16.0f * Scale,
        FLinearColor(1.0f, 0.96f, 0.83f, 0.95f));
    DrawPanel(CenterX - 8.0f * Scale, CenterY - 1.0f * Scale, 16.0f * Scale, 2.0f * Scale,
        FLinearColor(1.0f, 0.96f, 0.83f, 0.95f));

    const float SlotW = 92.0f * Scale;
    const float SlotH = 81.0f * Scale;
    const float Gap = 7.0f * Scale;
    const float BarW = 5.0f * SlotW + 4.0f * Gap;
    const float BarX = (ScreenW - BarW) * 0.5f;
    const float BarY = ScreenH - Margin - SlotH;
    for (int32 Slot = 0; Slot < HotbarSlotCount; ++Slot)
    {
        const EBlocklandBlock Type = HotbarBlocks[Slot];
        const float X = BarX + Slot * (SlotW + Gap);
        const bool bSelected = Slot == Player->GetSelectedSlot();
        DrawPanel(X - (bSelected ? 2.0f : 0.0f) * Scale, BarY - (bSelected ? 2.0f : 0.0f) * Scale,
            SlotW + (bSelected ? 4.0f : 0.0f) * Scale, SlotH + (bSelected ? 4.0f : 0.0f) * Scale,
            bSelected ? FLinearColor(0.94f, 0.77f, 0.32f, 0.96f) : FLinearColor(0.025f, 0.045f, 0.055f, 0.82f));
        DrawPanel(X + 6.0f * Scale, BarY + 6.0f * Scale, 25.0f * Scale, 25.0f * Scale,
            BlockColors[static_cast<int32>(Type)]);
        DrawLabel(FString::Printf(TEXT("%d"), Slot + 1), X + 9.0f * Scale, BarY + 8.0f * Scale,
            0.52f * Scale, FLinearColor::White);
        DrawLabel(BlockNames[static_cast<int32>(Type)], X + 8.0f * Scale, BarY + 39.0f * Scale,
            0.58f * Scale, FLinearColor::White);
        DrawLabel(FString::Printf(TEXT("x %d"), Player->GetInventoryCount(Type)),
            X + 8.0f * Scale, BarY + 59.0f * Scale, 0.54f * Scale,
            FLinearColor(0.85f, 0.89f, 0.82f));
    }

    DrawLabel(TEXT("WASD muovi   SPAZIO salta   SHIFT schiva"), Margin, ScreenH - 110.0f * Scale,
        0.62f * Scale, FLinearColor(0.92f, 0.94f, 0.88f));
    DrawLabel(TEXT("SINISTRO scava   DESTRO costruisci   F attacca   I zaino   N nuovo mondo"),
        Margin, ScreenH - 85.0f * Scale, 0.58f * Scale, FLinearColor(0.74f, 0.83f, 0.81f));

    if (Player->IsInventoryOpen())
    {
        const float PanelW = 520.0f * Scale;
        const float PanelH = 430.0f * Scale;
        const float PanelX = (ScreenW - PanelW) * 0.5f;
        const float PanelY = (ScreenH - PanelH) * 0.5f;
        DrawPanel(0.0f, 0.0f, ScreenW, ScreenH, FLinearColor(0.0f, 0.015f, 0.025f, 0.58f));
        DrawPanel(PanelX, PanelY, PanelW, PanelH, FLinearColor(0.035f, 0.07f, 0.085f, 0.97f));
        DrawLabel(TEXT("ZAINO"), PanelX + 30.0f * Scale, PanelY + 22.0f * Scale,
            1.15f * Scale, FLinearColor(0.95f, 0.85f, 0.59f));
        DrawLabel(TEXT("I  -  chiudi"), PanelX + PanelW - 130.0f * Scale, PanelY + 30.0f * Scale,
            0.62f * Scale, FLinearColor(0.72f, 0.82f, 0.80f));
        for (int32 Index = 0; Index < BlocklandBlockCount - 1; ++Index)
        {
            const float Y = PanelY + (85.0f + Index * 38.0f) * Scale;
            DrawPanel(PanelX + 25.0f * Scale, Y, 28.0f * Scale, 24.0f * Scale, BlockColors[Index]);
            DrawLabel(BlockNames[Index], PanelX + 68.0f * Scale, Y + 2.0f * Scale,
                0.69f * Scale, FLinearColor::White);
            DrawLabel(FString::Printf(TEXT("%d"), Player->GetInventoryCount(static_cast<EBlocklandBlock>(Index))),
                PanelX + PanelW - 70.0f * Scale, Y + 2.0f * Scale,
                0.69f * Scale, FLinearColor(0.94f, 0.83f, 0.52f));
        }
    }

    if (Player->GetStatusMessageTime() > 0.0f)
    {
        const float Width = 430.0f * Scale;
        const float X = (ScreenW - Width) * 0.5f;
        const float Y = ScreenH - 185.0f * Scale;
        DrawPanel(X, Y, Width, 38.0f * Scale, FLinearColor(0.035f, 0.07f, 0.085f, 0.86f));
        DrawLabel(Player->GetStatusMessage(), X + 16.0f * Scale, Y + 8.0f * Scale,
            0.68f * Scale, FLinearColor(0.96f, 0.92f, 0.78f));
    }
}
