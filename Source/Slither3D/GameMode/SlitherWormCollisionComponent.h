
#pragma once

#include "Components/ActorComponent.h"
#include "SlitherFoodTypes.h"
#include "SlitherWormSimulationTypes.h"

#include "SlitherWormCollisionComponent.generated.h"


UCLASS()
class USlitherWormCollisionComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    static constexpr double NumericalTolerance
        = UE_SMALL_NUMBER;

    TArray<FSlitherCollisionElement> Elements;
    TMap<FIntPoint, TArray<int32>> Cells;
    TSet<int32> CandidateIndices;

    TArray<int32> HeadElementIndices;
    TArray<int32> DeathContactIndices;
    TArray<int32> EarlierDeathContactIndices;

    TArray<FSlitherWormContact> Contacts;

private:

    bool HasServerAuthority() const;

    void AddElement(
        const FSlitherCollisionElement& Element);

    static void GetCellBounds(
        const FSlitherCollisionElement& Element,
        FIntPoint& OutMinimum,
        FIntPoint& OutMaximum);

    void FindHeadContacts(
        const int32 StepIndex,
        const TArray<FSlitherWormStepState>& StepStates,
        const TArray<FSlitherFoodCollisionEntry>& FoodEntries,
        TArray<FSlitherFoodContact>& OutFoodContacts);

    void ResolveDeaths(
        const TArray<FSlitherWormStepState>& StepStates,
        TArray<FSlitherWormDeathRecord>& OutDeaths);

    static void AddDeathRecord(
        const int32 StepIndex,
        const FSlitherWormContact& Contact,
        const TArray<FSlitherWormStepState>& StepStates,
        TArray<FSlitherWormDeathRecord>& OutDeaths);

    static bool CompareContacts(
        const FSlitherWormContact& First,
        const FSlitherWormContact& Second);

    static bool CompareDeaths(
        const FSlitherWormDeathRecord& First,
        const FSlitherWormDeathRecord& Second);

    static bool TryFindSphereContact(
        const FVector& FirstStart,
        const FVector& FirstEnd,
        const FVector& SecondStart,
        const FVector& SecondEnd,
        const double CombinedRadius,
        const double MaximumAlpha,
        double& OutContactAlpha);

public:

    USlitherWormCollisionComponent();

    void BuildCollisionGrid(
        const TArray<FSlitherWormStepState>& StepStates,
        const TArray<FSlitherFoodCollisionEntry>& FoodEntries);

    void FindContacts(
        const TArray<FSlitherWormStepState>& StepStates,
        const TArray<FSlitherFoodCollisionEntry>& FoodEntries,
        TArray<FSlitherFoodContact>& OutFoodContacts,
        TArray<FSlitherWormDeathRecord>& OutDeaths);
};
