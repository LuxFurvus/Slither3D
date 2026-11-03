
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherPlayerInputComponent.generated.h"


class ASlitherArena;
class ASlitherWormPawn;
class USlitherPlayerServerComponent;


UCLASS()
class USlitherPlayerInputComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    static constexpr double RayParallelTolerance
        = UE_KINDA_SMALL_NUMBER;

    bool BoostRequiresRelease = true;
    bool LocalWantsBoost = false;

    TWeakObjectPtr<ASlitherArena> InputArena;
    TWeakObjectPtr<ASlitherWormPawn> InputPawn;

    FVector LastAimDirection = FVector::ForwardVector;

    uint32 InputLifeId = 0;
    uint32 InputSequence = 0;

    double InputSendAccumulator = 0.0;

private:

    bool HasLocalInputFocus() const;

    void ResetLocalInput();

    ASlitherArena* FindInputArena();

    void UpdateLocalAim(
        const ASlitherWormPawn* WormPawn);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    USlitherPlayerInputComponent();

    void ConfigureLocalInput() const;

    void UpdateLocalInput(
        const float DeltaSeconds,
        USlitherPlayerServerComponent* ServerComponent);
};
