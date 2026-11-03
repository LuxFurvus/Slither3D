
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherPlayerUIComponent.generated.h"

class ASlitherPlayerController;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartReadyEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnSessionValueChangedEvent,
    const int32, NewValue);


UCLASS()
class USlitherPlayerUIComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    int32 LastSessionKills = 0;
    int32 LastSessionMass = 0;
    int32 LastSessionSeconds = 0;

    uint32 CurrentLifeId = 0;

    int32 CurrentKills = 0;
    int32 CurrentMass = 0;
    int32 CurrentSeconds = 0;

    double RespawnAvailableServerTime = -1.0;

    bool NewGame = true;
    bool SessionActive = false;
    bool StartRequestPending = false;
    bool ReadyEventSent = false;

public:

    UPROPERTY(BlueprintAssignable, Category = "SlitherUi")
    FOnStartReadyEvent OnReadyToRequestRestart;

    UPROPERTY(BlueprintAssignable, Category = "SlitherUi")
    FOnSessionValueChangedEvent OnKill;

    UPROPERTY(BlueprintAssignable, Category = "SlitherUi")
    FOnSessionValueChangedEvent OnMassIncreased;

    UPROPERTY(BlueprintAssignable, Category = "SlitherUi")
    FOnSessionValueChangedEvent OnTimeChanged;

private:

    ASlitherPlayerController* GetLocalPlayerController() const;

    bool CanRequestStartGame() const;

public:

    USlitherPlayerUIComponent();

    void UpdateLocalPresentation();

    void HandleSessionUpdate(
        const uint32 InLifeId,
        const int32 InKills,
        const int32 InMass,
        const int32 InTotalKillScore,
        const int32 InTotalMassScore,
        const int32 InSeconds,
        const bool InSessionActive,
        const double InRespawnAvailableServerTime);

    void HandleStartGameRequestResult(
        const uint32 ExpectedLifeId,
        const bool Accepted);

    void SetSessionResults(
        const int32 InResultKills,
        const int32 InResultMass,
        const int32 InResultSeconds);

    UFUNCTION(BlueprintCallable, Category = "SlitherUi")
    bool IsNewGame() const;

    UFUNCTION(BlueprintCallable, Category = "SlitherUi")
    void GetSessionResults(
        int32& FinalKills,
        int32& FinalMass,
        int32& FinalSeconds) const;

    UFUNCTION(BlueprintCallable, Category = "SlitherUi")
    void StartGame();
};