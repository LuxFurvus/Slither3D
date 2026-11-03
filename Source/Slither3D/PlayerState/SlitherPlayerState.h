
#pragma once

#include "GameFramework/PlayerState.h"

#include "Slither3D/SlitherPlayerLifeTypes.h"

#include "SlitherPlayerState.generated.h"

class USlitherPlayerLifeComponent;
class USlitherPlayerSessionStatsComponent;


UCLASS()
class ASlitherPlayerState : public APlayerState
{
    GENERATED_BODY()

private:

    UPROPERTY()
    TObjectPtr<USlitherPlayerLifeComponent> LifeComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherPlayerSessionStatsComponent> SessionStatsComponent = nullptr;

private:

    ASlitherPlayerState();

    void PublishSessionSeconds(
        const double ServerTime) const;

    void NotifySessionUpdate(
        const bool SessionActive) const;

public:

    void Tick(
        const float DeltaSeconds) override;

    uint32 GetLifeId() const;

    int32 GetMass() const;

    bool IsAlive() const;

    int32 GetKills() const;

    bool TryEndLife(
        const uint32 ExpectedLifeId) const;

    void FinalizeLifeDeath(
        const uint32 ExpectedLifeId,
        const double ServerTime);

    void AddKill(
        const uint32 ExpectedLifeId);

    int32 AddMass(
        const int32 Nutrition);

    const FSlitherPlayerLifeData& GetLifeData() const;

    bool IsWaitingForSpawn() const;

    bool TryQueueRespawn(
        const uint32 ExpectedLifeId,
        const double ServerTime);

    uint32 CalculateNextLifeId() const;

    bool TryBeginNextLife(
        const uint32 ExpectedLifeId,
        const double SpawnServerTime);

    double GetLifeDuration(
        const double ServerTime) const;

    double GetRespawnRemaining(
        const double ServerTime) const;

    bool TryQueueFirstSpawn();
};
