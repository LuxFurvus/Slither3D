
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SlitherWormSpawnTypes.h"

#include "SlitherWormSpawnComponent.generated.h"

class AController;
class ASlitherArena;
class ASlitherWormPawn;


UCLASS()
class USlitherWormSpawnComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    TArray<TWeakObjectPtr<ASlitherWormPawn>> WormPawns;
    TArray<FSlitherSpawnObstacle> Obstacles;
    TMap<FIntPoint, TArray<int32>> ObstacleCells;
    TSet<int32> CandidateIndices;

private:

    bool BuildObstacleGrid();

    void AddObstacle(
        const FVector& Location, const double Radius);

    static void GetCellBounds(
        const FVector2D& Center,
        const double Radius,
        FIntPoint& OutMinimum,
        FIntPoint& OutMaximum);

    bool CanPlaceElement(
        const ASlitherArena* Arena,
        const FVector2D& Center,
        const double Radius);

    TOptional<FTransform> TryBuildSpawnTransform(
        const ASlitherArena* Arena,
        const FVector2D& Center,
        const FVector2D& ArenaCenter);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    USlitherWormSpawnComponent();

    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& GetWormPawns() const;

    TOptional<FTransform> TryGetSpawnTransform(
        const ASlitherArena* Arena);

    void RegisterWormPawn(
        ASlitherWormPawn* WormPawn);

    void UnregisterWormPawn(
        const ASlitherWormPawn* WormPawn);

    ASlitherWormPawn* SpawnWorm(
        AController* NewPlayer,
        UClass* PawnClass,
        const FTransform& SpawnTransform) const;
};
