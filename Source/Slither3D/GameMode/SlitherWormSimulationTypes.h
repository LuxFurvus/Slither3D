
#pragma once

#include "CoreMinimal.h"
#include "Slither3D/WormPawn/SlitherWormMovementTypes.h"

class ASlitherPlayerState;
class ASlitherWormPawn;


enum class ESlitherDeathReason : uint8
{
    Head,
    Wall,
    Body
};


enum class ESlitherCollisionElementType : uint8
{
    Head,
    Body,
    Food
};


struct FSlitherWormStepState
{
public:

    TWeakObjectPtr<ASlitherWormPawn> WormPawn;
    TWeakObjectPtr<ASlitherPlayerState> PlayerState;
    uint32 LifeId = 0;
    int32 PlayerId = 0;
    int32 Mass = 0;
    FVector PreviousHeadLocation = FVector::ZeroVector;
    FVector CurrentHeadLocation = FVector::ZeroVector;
    double MovedDistance = 0.0;
    FSlitherWormMovementResult MovementResult;
    TArray<FVector> PreviousSegmentLocations;
    TArray<FVector> CurrentSegmentLocations;
};


struct FSlitherCollisionElement
{
public:

    ESlitherCollisionElementType Type = ESlitherCollisionElementType::Head;
    int32 OwnerStepIndex = INDEX_NONE;
    int32 SegmentIndex = INDEX_NONE;
    int32 FoodEntryIndex = INDEX_NONE;
    FVector PreviousLocation = FVector::ZeroVector;
    FVector CurrentLocation = FVector::ZeroVector;
    double Radius = 0.0;
};


struct FSlitherWormContact
{
public:

    int32 VictimStepIndex = INDEX_NONE;
    int32 OtherStepIndex = INDEX_NONE;
    int32 VictimPlayerId = 0;
    int32 OtherPlayerId = 0;
    int32 SegmentIndex = INDEX_NONE;
    double ContactAlpha = 0.0;
    ESlitherDeathReason Reason = ESlitherDeathReason::Wall;
};


struct FSlitherWormDeathRecord
{
public:

    TWeakObjectPtr<ASlitherWormPawn> WormPawn;
    TWeakObjectPtr<ASlitherPlayerState> PlayerState;
    TWeakObjectPtr<ASlitherPlayerState> KillerPlayerState;
    uint32 LifeId = 0;
    uint32 KillerLifeId = 0;
    int32 PlayerId = 0;
    int32 KillerPlayerId = INDEX_NONE;
    int32 Mass = 0;
    double ContactAlpha = 0.0;
    ESlitherDeathReason Reason = ESlitherDeathReason::Wall;
    FVector HeadLocation = FVector::ZeroVector;
    TArray<FVector> SegmentLocations;
};
