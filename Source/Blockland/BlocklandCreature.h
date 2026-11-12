#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlocklandCreature.generated.h"

class ABlocklandCharacter;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class BLOCKLAND_API ABlocklandCreature : public AActor
{
    GENERATED_BODY()

public:
    ABlocklandCreature();

    virtual void Tick(float DeltaSeconds) override;
    void ReceiveHit(float Damage);

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Head;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> EyeLeft;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> EyeRight;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> EyeMaterial;

    float Health = 60.0f;
    float AttackCooldown = 1.0f;
    float HitFlash = 0.0f;
    bool bUsesTintMaterial = false;
};
