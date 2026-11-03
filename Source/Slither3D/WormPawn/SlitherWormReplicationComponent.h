
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherWormMovementTypes.h"

#include "SlitherWormReplicationComponent.generated.h"

class ASlitherWormPawn;


UCLASS()
class USlitherWormReplicationComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    UPROPERTY(ReplicatedUsing = OnRep_LatestSnapshot)
    FSlitherWormSnapshot LatestSnapshot;

private:

    static constexpr uint32 MaxForwardSequenceDelta = MAX_uint32 / 2u;

    USlitherWormReplicationComponent();

    ASlitherWormPawn* GetWormPawn() const;

    void ReceiveSnapshot(
        const FSlitherWormSnapshot& Snapshot) const;

    UFUNCTION()
    void OnRep_LatestSnapshot() const;

public:

    void PublishSnapshot(
        const uint32 SimulationFrame,
        const double ServerTime,
        const float CurrentSpeed);

    void ProcessLatestSnapshot() const;

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
