
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SlitherFoodTypes.h"
#include "SlitherWormSimulationTypes.h"

#include "SlitherWormSimulationComponent.generated.h"

class ASlitherArena;
class USlitherFoodManagerComponent;
class USlitherWormCollisionComponent;
class USlitherWormSpawnComponent;
class ASlitherWormPawn;


UCLASS()
class USlitherWormSimulationComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    TArray<FSlitherWormStepState> StepStates;
    TArray<FSlitherFoodCollisionEntry> FoodEntries;
    TArray<FSlitherFoodContact> FoodContacts;
    TArray<FSlitherWormDeathRecord> DeathRecords;
    TArray<FSlitherWormDeathRecord> AcceptedDeaths;

    uint32 SimulationFrame = 0;

    double SimulationAccumulator = 0.0;
    double PublishAccumulator = 0.0;
    double LastSimulationTime = 0.0;

private:

    static bool CanUseStepState(
        const FSlitherWormStepState& State);

    void PublishSnapshots(
        const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns) const;

    void ApplyDeaths(
        const ASlitherArena* Arena,
        USlitherFoodManagerComponent* FoodManager,
        USlitherWormSpawnComponent* SpawnManager,
        const double ServerTime);

    void SimulateStep(
        const ASlitherArena* Arena,
        const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns,
        double StepSeconds,
        double ServerTime,
        USlitherFoodManagerComponent* FoodManager,
        USlitherWormCollisionComponent* CollisionManager,
        USlitherWormSpawnComponent* SpawnManager);

public:

    USlitherWormSimulationComponent();

    uint32 GetSimulationFrame() const;

    void UpdateSimulation(
        const float DeltaSeconds,
        const ASlitherArena* Arena,
        const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns,
        USlitherFoodManagerComponent* FoodManager,
        USlitherWormCollisionComponent* CollisionManager,
        USlitherWormSpawnComponent* SpawnManager);
};
