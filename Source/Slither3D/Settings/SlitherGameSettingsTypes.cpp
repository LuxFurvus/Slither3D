
#include "SlitherGameSettingsTypes.h"

#include "SlitherSettingsValidation.h"

static_assert(
    FSlitherSettingsCollision::CellSize > 0.0,
    "Collision cell size must be positive.");

static_assert(
    FSlitherSettingsCollision::ContactTimeTolerance > 0.0
        && FSlitherSettingsCollision::ContactTimeTolerance < 1.0,
    "Collision contact time tolerance must be between zero and one.");

static_assert(
    FSlitherSettingsFood::PreferredDeathFoodNutrition > 0,
    "Preferred death food nutrition must be positive.");


static_assert(
    FSlitherSettingsFood::MaxNaturalFoodSpawnsPerSubstep > 0,
    "Food spawn budget must be positive.");

static_assert(
    FSlitherSettingsFood::MaxPlacementAttemptsPerFood > 0,
    "Food placement attempt budget must be positive.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Nutrition
        < FSlitherSettingsFood::FoodTypes[1].Nutrition
        && FSlitherSettingsFood::FoodTypes[1].Nutrition
            < FSlitherSettingsFood::FoodTypes[2].Nutrition
        && FSlitherSettingsFood::FoodTypes[2].Nutrition
            < FSlitherSettingsFood::FoodTypes[3].Nutrition,
    "Food nutrition must be strictly increasing.");

static_assert(
    FSlitherSettingsValidation::IsColorNormalized(
        FSlitherSettingsFood::FoodTypes[0].FoodColor.R,
        FSlitherSettingsFood::FoodTypes[0].FoodColor.G,
        FSlitherSettingsFood::FoodTypes[0].FoodColor.B,
        FSlitherSettingsFood::FoodTypes[0].FoodColor.A),
    "FoodTypes[0] must use normalized opaque colors.");

static_assert(
    FSlitherSettingsValidation::IsColorNormalized(
        FSlitherSettingsFood::FoodTypes[1].FoodColor.R,
        FSlitherSettingsFood::FoodTypes[1].FoodColor.G,
        FSlitherSettingsFood::FoodTypes[1].FoodColor.B,
        FSlitherSettingsFood::FoodTypes[1].FoodColor.A),
    "FoodTypes[1] must use normalized opaque colors.");

static_assert(
    FSlitherSettingsValidation::IsColorNormalized(
        FSlitherSettingsFood::FoodTypes[2].FoodColor.R,
        FSlitherSettingsFood::FoodTypes[2].FoodColor.G,
        FSlitherSettingsFood::FoodTypes[2].FoodColor.B,
        FSlitherSettingsFood::FoodTypes[2].FoodColor.A),
    "FoodTypes[2] must use normalized opaque colors.");

static_assert(
    FSlitherSettingsValidation::IsColorNormalized(
        FSlitherSettingsFood::FoodTypes[3].FoodColor.R,
        FSlitherSettingsFood::FoodTypes[3].FoodColor.G,
        FSlitherSettingsFood::FoodTypes[3].FoodColor.B,
        FSlitherSettingsFood::FoodTypes[3].FoodColor.A),
    "FoodTypes[3] must use normalized opaque colors.");


static_assert(
    FSlitherSettingsFood::NaturalFoodCount >= 0,
    "Food.NaturalFoodCount must be nonnegative.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Nutrition >= 1,
    "FoodTypes[0].Nutrition must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Radius >= 1.0f,
    "FoodTypes[0].Radius must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].SpawnProbability >= 0.0f
        && FSlitherSettingsFood::FoodTypes[0].SpawnProbability <= 1.0f,
    "FoodTypes[0].SpawnProbability must be between 0 and 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].FoodColor.A >= 1.0f,
    "FoodTypes[0].FoodColor.A must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].Nutrition >= 1,
    "FoodTypes[1].Nutrition must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].Radius >= 1.0f,
    "FoodTypes[1].Radius must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].SpawnProbability >= 0.0f
        && FSlitherSettingsFood::FoodTypes[1].SpawnProbability <= 1.0f,
    "FoodTypes[1].SpawnProbability must be between 0 and 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].FoodColor.A >= 1.0f,
    "FoodTypes[1].FoodColor.A must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[2].Nutrition >= 1,
    "FoodTypes[2].Nutrition must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[2].Radius >= 1.0f,
    "FoodTypes[2].Radius must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[2].SpawnProbability >= 0.0f
        && FSlitherSettingsFood::FoodTypes[2].SpawnProbability <= 1.0f,
    "FoodTypes[2].SpawnProbability must be between 0 and 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[2].FoodColor.A >= 1.0f,
    "FoodTypes[2].FoodColor.A must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[3].Nutrition >= 1,
    "FoodTypes[3].Nutrition must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[3].Radius >= 1.0f,
    "FoodTypes[3].Radius must be at least 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[3].SpawnProbability >= 0.0f
        && FSlitherSettingsFood::FoodTypes[3].SpawnProbability <= 1.0f,
    "FoodTypes[3].SpawnProbability must be between 0 and 1.");

static_assert(
    FSlitherSettingsFood::FoodTypes[3].FoodColor.A >= 1.0f,
    "FoodTypes[3].FoodColor.A must be at least 1.");

static_assert(
    FSlitherSettingsFood::DeathFoodMassFraction >= 0.001f
        && FSlitherSettingsFood::DeathFoodMassFraction <= 1.0f,
    "Food.DeathFoodMassFraction must be between 0.001 and 1.");

static_assert(
    FSlitherSettingsFood::DeathFoodLifetime >= 0.001f,
    "Food.DeathFoodLifetime must be at least 0.001 seconds.");

static_assert(
    FSlitherSettingsFood::MaxDeathFoodCount >= 0,
    "Food.MaxDeathFoodCount must be nonnegative.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Nutrition
        != FSlitherSettingsFood::FoodTypes[1].Nutrition,
    "FoodTypes[0].Nutrition and FoodTypes[1].Nutrition must differ.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Nutrition
        != FSlitherSettingsFood::FoodTypes[2].Nutrition,
    "FoodTypes[0].Nutrition and FoodTypes[2].Nutrition must differ.");

static_assert(
    FSlitherSettingsFood::FoodTypes[0].Nutrition
        != FSlitherSettingsFood::FoodTypes[3].Nutrition,
    "FoodTypes[0].Nutrition and FoodTypes[3].Nutrition must differ.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].Nutrition
        != FSlitherSettingsFood::FoodTypes[2].Nutrition,
    "FoodTypes[1].Nutrition and FoodTypes[2].Nutrition must differ.");

static_assert(
    FSlitherSettingsFood::FoodTypes[1].Nutrition
        != FSlitherSettingsFood::FoodTypes[3].Nutrition,
    "FoodTypes[1].Nutrition and FoodTypes[3].Nutrition must differ.");

static_assert(
    FSlitherSettingsFood::FoodTypes[2].Nutrition
        != FSlitherSettingsFood::FoodTypes[3].Nutrition,
    "FoodTypes[2].Nutrition and FoodTypes[3].Nutrition must differ.");

static_assert(
    FSlitherSettingsValidation::IsNearlyEqual(
        0.0
            + FSlitherSettingsFood::FoodTypes[0].SpawnProbability
            + FSlitherSettingsFood::FoodTypes[1].SpawnProbability
            + FSlitherSettingsFood::FoodTypes[2].SpawnProbability
            + FSlitherSettingsFood::FoodTypes[3].SpawnProbability,
        1.0,
        0.0001),
    "FoodTypes probabilities must sum to 1.");

static_assert(
    FSlitherSettingsFood::DeathFoodMassFraction <= 0.0f
        || FSlitherSettingsFood::MaxDeathFoodCount > 0,
    "Positive DeathFoodMassFraction requires positive MaxDeathFoodCount.");

static_assert(
    FSlitherSettingsBody::PathSampleDistance > 0.0f
        && FSlitherSettingsBody::SegmentSpacing
            >= FSlitherSettingsBody::PathSampleDistance,
    "Body spacing must cover a positive path sample interval.");

static_assert(
    FSlitherSettingsBody::HistoryReserveDistance >= 0.0
        && FSlitherSettingsBody::UnitSphereRadius > 0.0,
    "Body history reserve and sphere radius must be valid.");

static_assert(
    FSlitherSettingsGrowth::InitialElementCount > 1
        && FSlitherSettingsGrowth::MaxElementCount
            >= FSlitherSettingsGrowth::InitialElementCount,
    "Worm element limits must include a head and body.");

static_assert(
    FSlitherSettingsBody::MaxClientPathPointCount
        >= FSlitherSettingsGrowth::MaxServerPathPointCount,
    "Client path cache must fit the complete server history.");
