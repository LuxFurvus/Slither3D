
#include "SlitherPlayerSessionStatsComponent.h"

#include "Slither3D/SlitherPlayerLifeTypes.h"


USlitherPlayerSessionStatsComponent::USlitherPlayerSessionStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void USlitherPlayerSessionStatsComponent::ResetForNextLife()
{
    PublishedSeconds = 0;
}

void USlitherPlayerSessionStatsComponent::RecordCompletedLife(
    const FSlitherPlayerLifeData& CompletedLife)
{
    const int32 SessionMassScore
        = CompletedLife.Mass - FSlitherSettingsGrowth::InitialMass;
    const int32 RemainingMassScore
        = MAX_int32 - TotalMassScore;
    const int32 AwardedMassScore
        = FMath::Min(SessionMassScore, RemainingMassScore);
    TotalMassScore += AwardedMassScore;

    const int32 RemainingKillScore
        = MAX_int32 - TotalKillScore;
    const int32 AwardedKillScore
        = FMath::Min(CompletedLife.Kills, RemainingKillScore);
    TotalKillScore += AwardedKillScore;
}

bool USlitherPlayerSessionStatsComponent::TryAdvancePublishedSeconds(
    const double LifeDuration)
{
    const int32 CompleteSeconds
        = FMath::FloorToInt(LifeDuration);

    if (PublishedSeconds >= CompleteSeconds)
    {
        return false;
    }

    ++PublishedSeconds;

    return true;
}

int32 USlitherPlayerSessionStatsComponent::CalculateDisplayMass(
    const FSlitherPlayerLifeData& CurrentLife)
{
    const int32 AdditionalMass
        = CurrentLife.Mass - FSlitherSettingsGrowth::InitialMass;

    const int32 ResultDisplayMass
        = FMath::Max(AdditionalMass, 0);

    return ResultDisplayMass;
}

int32 USlitherPlayerSessionStatsComponent::GetTotalMassScore() const
{
    return TotalMassScore;
}

int32 USlitherPlayerSessionStatsComponent::GetTotalKillScore() const
{
    return TotalKillScore;
}

int32 USlitherPlayerSessionStatsComponent::GetPublishedSeconds() const
{
    return PublishedSeconds;
}
