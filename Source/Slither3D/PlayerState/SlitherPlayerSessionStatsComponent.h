
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherPlayerSessionStatsComponent.generated.h"

struct FSlitherPlayerLifeData;


UCLASS()
class USlitherPlayerSessionStatsComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    int32 PublishedSeconds = 0;

    int32 TotalMassScore = 0;

    int32 TotalKillScore = 0;

private:

    USlitherPlayerSessionStatsComponent();

public:

    void ResetForNextLife();

    void RecordCompletedLife(
        const FSlitherPlayerLifeData& CompletedLife);

    bool TryAdvancePublishedSeconds(
        const double LifeDuration);

    static int32 CalculateDisplayMass(
        const FSlitherPlayerLifeData& CurrentLife);

    int32 GetTotalMassScore() const;

    int32 GetTotalKillScore() const;

    int32 GetPublishedSeconds() const;
};
