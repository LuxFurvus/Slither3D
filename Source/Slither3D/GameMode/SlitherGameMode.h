
#pragma once

#include "GameFramework/GameMode.h"
#include "SlitherWormSpawnTypes.h"

#include "SlitherGameMode.generated.h"

class ASlitherArena;
class ASlitherPlayerState;
class USlitherFoodManagerComponent;
class USlitherWormCollisionComponent;
class USlitherWormSimulationComponent;
class USlitherWormSpawnComponent;


UCLASS()
class ASlitherGameMode : public AGameMode
{
    GENERATED_BODY()
private:

    bool ArenaInitializationAttempted = false;
    bool ProcessingSpawnRequests = false;
    bool EndingPlay = false;

    TArray<FSlitherSpawnRequest> SpawnRequests;

    UPROPERTY()
    TObjectPtr<ASlitherArena> ArenaActor = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherWormSpawnComponent> WormSpawnComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Simulation")
    TObjectPtr<USlitherWormSimulationComponent> WormSimulationComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Collision")
    TObjectPtr<USlitherWormCollisionComponent> WormCollisionComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Food")
    TObjectPtr<USlitherFoodManagerComponent> FoodManagerComponent = nullptr;

private:

    ASlitherGameMode();

    ASlitherArena* GetSingletonArena() const;

    void InitializeArena();

    void StartPlay() override;

    double GetSpawnServerTime() const;

    ASlitherPlayerState* GetSpawnPlayerState(
        const AController* InController) const;

    int32 FindSpawnRequest(
        const AController* Controller) const;

    bool IsSpawnRequestCurrent(
        const FSlitherSpawnRequest& Request) const;

    bool TrySpawnRequest(
        const FSlitherSpawnRequest& Request);

    void ProcessSpawnRequests(
        const double ServerTime);

    static void CheckArenaRotationAndScale(
        const AActor* InArenaActor);

    static void DiscardSpawnedPawn(
        AController* Controller,
        APawn* SpawnedPawn);

    static bool CompareSpawnRequests(
        const FSlitherSpawnRequest& First,
        const FSlitherSpawnRequest& Second);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    void Tick(
        const float DeltaSeconds) override;

    bool TryRequestStartGame(
        AController* InController, const uint32 ExpectedLifeId);

    void Logout(
        AController* Exiting) override;

    APawn* SpawnDefaultPawnAtTransform_Implementation(
        AController* NewPlayer,
        const FTransform& SpawnTransform) override;
};
