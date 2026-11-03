
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherFoodTypes.h"

#include "SlitherFoodManagerComponent.generated.h"

class ASlitherArena;
class ASlitherFood;
struct FSlitherWormDeathRecord;


UCLASS()
class USlitherFoodManagerComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    TMap<uint64, TWeakObjectPtr<ASlitherFood>> Foods;

    TSet<uint64> NaturalFoodIds;

    TArray<uint64> DeathFoodIdsInCreationOrder;

    TArray<FSlitherPendingDeathFood> PendingDeathFood;

    double LastDeathFoodWarningTime = -1.0;

    uint64 NextFoodId = 1;

    bool EndingPlay = false;

    UPROPERTY(EditDefaultsOnly, Category = "Food")
    TSubclassOf<ASlitherFood> FoodClass;

private:

    bool HasServerAuthority() const;

    UClass* GetRequiredFoodClass() const;

    void RemoveUnavailableFood();

    void EnforceDeathFoodLimit();

    static int32 ChooseNaturalNutrition();

    void RegisterFood(
        ASlitherFood* Food,
        const bool NaturalFood);

    static bool CanPlaceFood(
        const ASlitherArena* Arena,
        const FVector2D& Center,
        const double Radius,
        const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns);

    static bool CompareContacts(
        const FSlitherFoodContact& First,
        const FSlitherFoodContact& Second);

protected:

    void BeginPlay() override;

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    USlitherFoodManagerComponent();

    void SpawnPendingDeathFood();

    int32 GetNaturalFoodCount() const;

    int32 GetTotalFoodCount() const;

    int32 GetDeathFoodCount() const;

    void UnregisterFood(
        const uint64 FoodId);

    ASlitherFood* SpawnFood(
        const int32 Nutrition,
        const FVector& Location,
        const bool NaturalFood);

    TArray<FSlitherFoodCollisionEntry> GatherCollisionEntries();

    void CollectFood(
        TArray<FSlitherFoodContact>& Contacts);

    void QueueDeathFood(
        const ASlitherArena* Arena,
        const FSlitherWormDeathRecord& Death);

    void ReplenishNaturalFood(
        const ASlitherArena* Arena,
        const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns);
};
