
#pragma once

#include "CoreMinimal.h"

#include "SlitherFoodType.h"


struct FSlitherSettingsArena
{
public:

    static constexpr double ArenaWidth = 12000.0;
    static constexpr double ArenaHeight = 12000.0;

    static constexpr double FloorThickness = 100.0;

    static constexpr double WallThickness = 100.0;

    static constexpr double WallHeight = 200.0;
};

struct FSlitherSettingsRespawn
{
public:

    static constexpr float RespawnDelay = 3.0f;

    static constexpr double SpawnClearance = 100.0;
    static constexpr int32 RandomSpawnAttemptCount = 32;
    static constexpr double SpawnRetryInterval = 0.5;
    static constexpr double SpawnGridSpacing = 200.0;
};

struct FSlitherSettingsNetwork
{
public:

    static constexpr float ServerSimulationRate = 30.0f;

    static constexpr double ServerSimulationInterval = 1.0 / ServerSimulationRate;

    static constexpr int32 MaxCatchUpSubsteps = 4;

    static constexpr float InputSendRate = 20.0f;

    static constexpr double InputSendInterval = 1.0 / InputSendRate;

    static constexpr float InputTimeout = 0.5f;

    static constexpr int32 InputBurstCapacity = 4;

    static constexpr float WormStatePublishRate = 20.0f;

    static constexpr double WormStatePublishInterval = 1.0 / WormStatePublishRate;

    static constexpr float ClientInterpolationDelay = 0.1f;

    static constexpr int32 MaxSnapshotCount = 32;
};

struct FSlitherSettingsCollision
{
public:

    static constexpr double CellSize = 200.0;

    static constexpr double ContactTimeTolerance = 0.000001;
};


struct FSlitherSettingsMovement
{
public:

    static constexpr float BaseSpeed = 600.0f;

    static constexpr float BoostSpeed = 900.0f;

    static constexpr float MaxTurnRate = 180.0f;

    static constexpr float AimDeadZone = 1.0f;
};

struct FSlitherSettingsBody
{
public:

    static constexpr float HeadRadius = 32.0f;

    static constexpr float BodySegmentRadius = 28.0f;

    static constexpr float SegmentSpacing = 50.0f;

    static constexpr float PathSampleDistance = 25.0f;

    static constexpr double UnitSphereRadius = 50.0;

    static constexpr double HistoryReserveDistance = 500.0;

    static constexpr int32 MaxClientPathPointCount = 512;
};

struct FSlitherSettingsGrowth
{
public:

    static constexpr int32 InitialMass = 100;

    static constexpr int32 InitialElementCount = 8;

    static constexpr int32 MaxElementCount = 128;

    static constexpr float LengthPerMassUnit = 5.0f;

    static constexpr int32 MaxMass = 1300;

    static constexpr double InitialBodyLength
        = (InitialElementCount - 1) * FSlitherSettingsBody::SegmentSpacing;

    static constexpr double MaxBodyLength
        = (MaxElementCount - 1) * FSlitherSettingsBody::SegmentSpacing;

    static constexpr int32 MaxServerPathPointCount = static_cast<int32>(
        (MaxBodyLength + FSlitherSettingsBody::HistoryReserveDistance)
        / FSlitherSettingsBody::PathSampleDistance) + 2;
};

struct FSlitherSettingsFood
{
private:

    static constexpr FLinearColor GreenColor{0.4f, 0.8f, 0.4f, 1.0f};
    static constexpr FLinearColor BlueColor{0.4f, 0.8f, 1.0f, 1.0f};
    static constexpr FLinearColor RedColor{1.0f, 0.4f, 0.4f, 1.0f};
    static constexpr FLinearColor GoldColor{1.0f, 0.8f, 0.2f, 1.0f};

public:

    static constexpr int32 NaturalFoodCount = 100;
    static constexpr int32 MaxNaturalFoodSpawnsPerSubstep = 20;
    static constexpr int32 MaxPlacementAttemptsPerFood = 5;

    static constexpr FSlitherFoodType FoodTypes[4] =
    {
        {.Nutrition = 5, .SpawnProbability = 0.60f, .Radius = 10.0f, .FoodColor = GreenColor},
        {.Nutrition = 10, .SpawnProbability = 0.25f, .Radius = 14.0f, .FoodColor = BlueColor},
        {.Nutrition = 25, .SpawnProbability = 0.10f, .Radius = 18.0f, .FoodColor = RedColor},
        {.Nutrition = 50, .SpawnProbability = 0.05f, .Radius = 24.0f, .FoodColor = GoldColor}
    };

    static constexpr int32 PreferredDeathFoodNutrition = 10;

    static constexpr float DeathFoodMassFraction = 0.5f;

    static constexpr float DeathFoodLifetime = 30.0f;

    static constexpr int32 MaxDeathFoodCount = 1000;
};
