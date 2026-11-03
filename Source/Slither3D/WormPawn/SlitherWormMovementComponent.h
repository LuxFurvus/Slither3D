
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherWormMovementTypes.h"

#include "SlitherWormMovementComponent.generated.h"

class ASlitherArena;
class ASlitherWormPawn;


UCLASS()
class USlitherWormMovementComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    static constexpr uint32 MaxForwardSequenceDelta = MAX_uint32 / 2u;

    FSlitherWormInputState InputState;

    float CurrentSpeed = 0.0f;

private:

    USlitherWormMovementComponent();

    ASlitherWormPawn* GetWormPawn() const;

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    const FSlitherWormInputState& GetInputState() const;

    float GetCurrentSpeed() const;

    void ResetInputState();

    bool SubmitInput(
        const uint32 InputLifeId,
        const uint32 InputSequence,
        const FVector& AimDirection,
        const bool WantsBoost,
        const double InputTime);

    void SimulateMovement(
        const ASlitherArena* Arena,
        const double StepSeconds,
        const double ServerTime,
        FSlitherWormMovementResult& OutResult);
};
