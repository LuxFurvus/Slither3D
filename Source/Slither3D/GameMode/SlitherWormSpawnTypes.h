#pragma once

#include "CoreMinimal.h"

class AController;
class ASlitherPlayerState;


struct FSlitherSpawnObstacle
{
public:

    FVector2D Center = FVector2D::ZeroVector;
    double Radius = 0.0;
};


struct FSlitherSpawnRequest
{
public:

    TWeakObjectPtr<AController> Controller;
    TWeakObjectPtr<ASlitherPlayerState> PlayerState;
    uint32 ExpectedLifeId = 0;
    double NextAttemptServerTime = 0.0;
    bool Processing = false;
};
