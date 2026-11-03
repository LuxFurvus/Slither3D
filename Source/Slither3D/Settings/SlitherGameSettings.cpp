
#include "SlitherGameSettings.h"
#include "SlitherGameSettingsTypes.h"
#include "SlitherSettingsValidation.h"

#include <cfloat>


constexpr bool FSlitherGameSettings::HasValidMovementRelations()
{
    constexpr bool Result
        = FSlitherSettingsMovement::BoostSpeed
        > FSlitherSettingsMovement::BaseSpeed;

    return Result;
}

constexpr bool FSlitherGameSettings::HasValidGeometryRelations()
{
    constexpr double HeadRadius = FSlitherSettingsBody::HeadRadius;
    constexpr double BodySegmentRadius = FSlitherSettingsBody::BodySegmentRadius;
    constexpr double HeadDiameter = HeadRadius * 2.0;
    constexpr double BodyDiameter = BodySegmentRadius * 2.0;

    constexpr bool HeadFits =
        HeadDiameter < FSlitherSettingsArena::ArenaWidth
        && HeadDiameter < FSlitherSettingsArena::ArenaHeight;

    constexpr bool BodyFits =
        BodyDiameter < FSlitherSettingsArena::ArenaWidth
        && BodyDiameter < FSlitherSettingsArena::ArenaHeight;

    constexpr bool PathSpacingValid =
        FSlitherSettingsBody::PathSampleDistance
        <= FSlitherSettingsBody::SegmentSpacing;

    if (!HeadFits || !BodyFits || !PathSpacingValid)
    {
        return false;
    }

    for (const FSlitherFoodType& FoodType : FSlitherSettingsFood::FoodTypes)
    {
        const double FoodRadius = FoodType.Radius;
        const double FoodDiameter = FoodRadius * 2.0;

        const bool FoodFits =
            FoodDiameter < FSlitherSettingsArena::ArenaWidth
            && FoodDiameter < FSlitherSettingsArena::ArenaHeight;

        if (const bool FoodClearsFloor = FoodRadius <= HeadRadius;
            !FoodFits || !FoodClearsFloor)
        {
            return false;
        }
    }

    return true;
}

constexpr bool FSlitherGameSettings::HasValidGrowthRelations()
{
    if (FSlitherSettingsGrowth::InitialMass > FSlitherSettingsGrowth::MaxMass
        || FSlitherSettingsGrowth::InitialElementCount
        > FSlitherSettingsGrowth::MaxElementCount)
    {
        return false;
    }

    constexpr double InitialElementCount = FSlitherSettingsGrowth::InitialElementCount;
    constexpr double MaxElementCount = FSlitherSettingsGrowth::MaxElementCount;
    constexpr double SegmentSpacing = FSlitherSettingsBody::SegmentSpacing;

    constexpr double InitialLength = (InitialElementCount - 1.0) * SegmentSpacing;
    constexpr double MaximumLength = (MaxElementCount - 1.0) * SegmentSpacing;

    constexpr double InitialMass = FSlitherSettingsGrowth::InitialMass;
    constexpr double MaxMass = FSlitherSettingsGrowth::MaxMass;
    constexpr double AdditionalMass = MaxMass - InitialMass;

    constexpr double AddedLength =
        AdditionalMass * FSlitherSettingsGrowth::LengthPerMassUnit;

    constexpr double LengthAtMaximumMass = InitialLength + AddedLength;

    constexpr bool LimitsMatch = FSlitherSettingsValidation::IsNearlyEqual(
        LengthAtMaximumMass, MaximumLength, 0.01);

    return LimitsMatch;
}


static_assert(
    FSlitherSettingsArena::ArenaWidth >= 1000.0,
    "Arena.ArenaWidth must be at least 1000.");

static_assert(
    FSlitherSettingsArena::ArenaHeight >= 1000.0,
    "Arena.ArenaHeight must be at least 1000.");

static_assert(
    FSlitherSettingsArena::FloorThickness > 0.0,
    "Arena.FloorThickness must be positive.");

static_assert(
    FSlitherSettingsArena::WallThickness > 0.0,
    "Arena.WallThickness must be positive.");

static_assert(
    FSlitherSettingsArena::WallHeight > 0.0,
    "Arena.WallHeight must be positive.");

static_assert(
    FSlitherSettingsArena::WallHeight >= 2.0 * FSlitherSettingsBody::HeadRadius,
    "Arena.WallHeight must cover the head above the floor.");

static_assert(
    FSlitherSettingsRespawn::RespawnDelay >= 0.001f,
    "Respawn.RespawnDelay must be at least 0.001 seconds.");

static_assert(
    FSlitherSettingsRespawn::SpawnClearance >= 0.0,
    "Respawn.SpawnClearance must not be negative.");

static_assert(
    FSlitherSettingsRespawn::RandomSpawnAttemptCount > 0,
    "Respawn.RandomSpawnAttemptCount must be positive.");

static_assert(
    FSlitherSettingsRespawn::SpawnRetryInterval > 0.0,
    "Respawn.SpawnRetryInterval must be positive.");

static_assert(
    FSlitherSettingsRespawn::SpawnGridSpacing > 0.0,
    "Respawn.SpawnGridSpacing must be positive.");

static_assert(
    FSlitherSettingsNetwork::ServerSimulationRate >= 10.0f,
    "Network.ServerSimulationRate must be at least 10.");

static_assert(
    FSlitherSettingsNetwork::InputSendRate >= 10.0f,
    "Network.InputSendRate must be at least 10.");

static_assert(
    FSlitherSettingsNetwork::WormStatePublishRate >= 10.0f,
    "Network.WormStatePublishRate must be at least 10.");

static_assert(
    FSlitherSettingsNetwork::ClientInterpolationDelay >= 0.1f,
    "Network.ClientInterpolationDelay must be at least 0.1 seconds.");

static_assert(
    FSlitherSettingsNetwork::ServerSimulationInterval > 0.0,
    "Network.ServerSimulationInterval must be positive.");

static_assert(
    FSlitherSettingsNetwork::InputSendInterval > 0.0,
    "Network.InputSendInterval must be positive.");

static_assert(
    FSlitherSettingsNetwork::WormStatePublishInterval > 0.0,
    "Network.WormStatePublishInterval must be positive.");

static_assert(
    FSlitherSettingsNetwork::MaxCatchUpSubsteps > 0,
    "Network.MaxCatchUpSubsteps must be positive.");

static_assert(
    FSlitherSettingsNetwork::InputTimeout > FSlitherSettingsNetwork::InputSendInterval
        && FSlitherSettingsNetwork::InputTimeout <= FLT_MAX,
    "Network.InputTimeout must be exceed InputSendInterval.");

static_assert(
    FSlitherSettingsNetwork::InputBurstCapacity > 0,
    "Network.InputBurstCapacity must be positive.");

static_assert(
    FSlitherSettingsNetwork::MaxSnapshotCount >= 2,
    "Network.MaxSnapshotCount must be at least 2.");

static_assert(
    FSlitherSettingsMovement::AimDeadZone > 0.0f
        && FSlitherSettingsMovement::AimDeadZone <= FLT_MAX,
    "Movement.AimDeadZone must be positive.");

static_assert(
    FSlitherSettingsMovement::BaseSpeed >= 100.0f,
    "Movement.BaseSpeed must be at least 100.");

static_assert(
    FSlitherSettingsMovement::BoostSpeed >= 100.0f,
    "Movement.BoostSpeed must be at least 100.");

static_assert(
    FSlitherSettingsMovement::MaxTurnRate >= 1.0f,
    "Movement.MaxTurnRate must be at least 1.");

static_assert(
    FSlitherSettingsBody::HeadRadius >= 1.0f,
    "Body.HeadRadius must be at least 1.");

static_assert(
    FSlitherSettingsBody::BodySegmentRadius >= 1.0f,
    "Body.BodySegmentRadius must be at least 1.");

static_assert(
    FSlitherSettingsBody::BodySegmentRadius <= FSlitherSettingsBody::HeadRadius,
    "Body.BodySegmentRadius must not exceed HeadRadius to clear the floor.");

static_assert(
    FSlitherSettingsBody::SegmentSpacing >= 1.0f,
    "Body.SegmentSpacing must be at least 1.");

static_assert(
    FSlitherSettingsBody::PathSampleDistance >= 1.0f,
    "Body.PathSampleDistance must be at least 1.");

static_assert(
    FSlitherSettingsGrowth::LengthPerMassUnit >= 0.001f,
    "Growth.LengthPerMassUnit must be at least 0.001.");

static_assert(
    FSlitherSettingsGrowth::InitialMass >= 1,
    "Growth.InitialMass must be at least 1.");

static_assert(
    FSlitherSettingsGrowth::MaxMass >= 1,
    "Growth.MaxMass must be at least 1.");

static_assert(
    FSlitherSettingsGrowth::InitialElementCount >= 2
        && FSlitherSettingsGrowth::InitialElementCount <= 128,
    "Growth.InitialElementCount must be between 2 and 128.");

static_assert(
    FSlitherSettingsGrowth::MaxElementCount >= 2
        && FSlitherSettingsGrowth::MaxElementCount <= 128,
    "Growth.MaxElementCount must be between 2 and 128.");

static_assert(
    FSlitherGameSettings::HasValidMovementRelations(),
    "Movement.BoostSpeed must exceed Movement.BaseSpeed.");

static_assert(
    FSlitherGameSettings::HasValidGeometryRelations(),
    "Body.PathSampleDistance must not exceed SegmentSpacing.");

static_assert(
    FSlitherGameSettings::HasValidGrowthRelations(),
    "Growth initial values must not exceed maxima; maximum mass must produce maximum body length.");
