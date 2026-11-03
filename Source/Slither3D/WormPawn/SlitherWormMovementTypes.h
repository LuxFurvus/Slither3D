
#pragma once

#include "CoreMinimal.h"

#include "SlitherWormMovementTypes.generated.h"


USTRUCT()
struct FSlitherWormSnapshot
{
    GENERATED_BODY()

public:

    UPROPERTY()
    uint32 LifeId = 0;

    UPROPERTY()
    uint32 SimulationFrame = 0;

    UPROPERTY()
    double ServerTime = 0.0;

    UPROPERTY()
    FVector HeadLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector HeadDirection = FVector::ForwardVector;

    UPROPERTY()
    float Speed = 0.0f;

    UPROPERTY()
    double HeadPathDistance = 0.0;

    UPROPERTY()
    double BodyLength = 0.0;

    UPROPERTY()
    uint64 RequiredPathPointSequence = 0;
};


struct FSlitherWormMovementResult
{
public:

    FVector PreviousHeadLocation = FVector::ZeroVector;
    FVector IntendedHeadLocation = FVector::ZeroVector;
    FVector CurrentHeadLocation = FVector::ZeroVector;
    bool HitWall = false;
    double WallContactAlpha = 1.0;
};


struct FSlitherWormInputState
{
public:

    FVector AimDirection = FVector::ForwardVector;

    bool WantsBoost = false;

    bool HasInput = false;

    uint32 LastInputSequence = 0;

    double LastInputTime = 0.0;
};
