
#pragma once

#include "GameFramework/Actor.h"

#include "SlitherFood.generated.h"

class ASlitherPlayerState;
class USceneComponent;
class USlitherFoodManagerComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;


UCLASS()
class ASlitherFood : public AActor
{
    GENERATED_BODY()

private:

    uint64 FoodId = 0;

    bool Consumed = false;

    const FName ColorParameterName = FName("FoodColor");

    TWeakObjectPtr<USlitherFoodManagerComponent> FoodManager;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_Nutrition)
    int32 Nutrition = 0;

    UPROPERTY(VisibleAnywhere, Category = "Food")
    TObjectPtr<USceneComponent> FoodRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Food")
    TObjectPtr<UStaticMeshComponent> FoodMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Food")
    TObjectPtr<UMaterialInterface> FoodMaterial = nullptr;

private:

    void UpdateAppearance();

    UFUNCTION()
    void OnRep_Nutrition();

protected:

    void BeginPlay() override;

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    ASlitherFood();

    int32 GetNutrition() const;

    uint64 GetFoodId() const;

    bool IsAvailable() const;

    void InitializeFood(
        const int32 InNutrition,
        const uint64 InFoodId,
        USlitherFoodManagerComponent* InManager);

    bool TryConsumeBy(
        ASlitherPlayerState* PlayerState,
        int32& OutAcceptedMass);

    static void CalculateAppearance(
        const int32 InNutrition,
        double& OutRadius,
        FLinearColor& OutColor);

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
