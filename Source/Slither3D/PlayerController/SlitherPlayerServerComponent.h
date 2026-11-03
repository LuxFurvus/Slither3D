
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherPlayerServerComponent.generated.h"


UCLASS()
class USlitherPlayerServerComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    static constexpr double DirectionZTolerance = 0.001;
    static constexpr double MinimumDirectionLength = 0.99;
    static constexpr double MaximumDirectionLength = 1.01;

    double InputBudgetTime = -1.0;
    double InputTokens = 0.0;

private:

    bool ConsumeInputBudget(
        const double InputTime);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    USlitherPlayerServerComponent();

    static bool TryNormalizeAimDirection(
        const FVector& AimDirection,
        FVector& NormalizedDirection);

    UFUNCTION(Client, Reliable)
    void ClientSessionUpdate(
        const uint32 LifeId,
        const int32 Kills,
        const int32 Mass,
        const int32 TotalKillScore,
        const int32 TotalMassScore,
        const int32 Seconds,
        const bool SessionActive,
        const double RespawnAvailableServerTime);

    UFUNCTION(Client, Reliable)
    void ClientStartGameRequestResult(
        const uint32 ExpectedLifeId,
        const bool Accepted);

    UFUNCTION(Server, Reliable)
    void ServerRequestStartGame(
        const uint32 ExpectedLifeId);

    UFUNCTION(Server, Unreliable)
    void ServerSubmitInput(
        uint32 LifeId,
        uint32 Sequence,
        FVector AimDirection,
        bool WantsBoost);
};
