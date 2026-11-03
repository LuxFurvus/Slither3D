
#pragma once

#include "Components/ActorComponent.h"

#include "Slither3D/SlitherPlayerLifeTypes.h"

#include "SlitherPlayerLifeComponent.generated.h"


UCLASS()
class USlitherPlayerLifeComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    UPROPERTY(Replicated)
    FSlitherPlayerLifeData LifeData;

private:

    USlitherPlayerLifeComponent();

    bool HasOwnerAuthority() const;

public:

    uint32 GetLifeId() const;

    int32 GetMass() const;

    bool IsAlive() const;

    int32 GetKills() const;

    bool TryEndLife(
        const uint32 ExpectedLifeId);

    bool FinalizeLifeDeath(
        const uint32 ExpectedLifeId,
        const double ServerTime);

    bool AddKill(
        const uint32 ExpectedLifeId);

    int32 AddMass(
        const int32 Nutrition);

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
