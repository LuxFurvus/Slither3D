
#pragma once

#include "CoreMinimal.h"

class ASlitherFood;
class ASlitherPlayerState;
class ASlitherWormPawn;


struct FSlitherFoodCollisionEntry
{
public:

    TWeakObjectPtr<ASlitherFood> Food;
    uint64 FoodId = 0;
    FVector Location = FVector::ZeroVector;
    double Radius = 0.0;
};


struct FSlitherPendingDeathFood
{
public:

    int32 Nutrition = 0;
    FVector Location = FVector::ZeroVector;
};


struct FSlitherFoodContact
{
public:

    int32 PlayerId = 0;
    uint32 LifeId = 0;
    uint64 FoodId = 0;
    double ContactAlpha = 0.0;
    TWeakObjectPtr<ASlitherFood> Food;
    TWeakObjectPtr<ASlitherWormPawn> WormPawn;
    TWeakObjectPtr<ASlitherPlayerState> PlayerState;
};
