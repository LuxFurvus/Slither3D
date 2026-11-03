
#include "SlitherPlayerUIComponent.h"

#include "SlitherPlayerController.h"
#include "SlitherPlayerServerComponent.h"
#include "Slither3D/SlitherGameState.h"
#include "Slither3D/SlitherHUD.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"


ASlitherPlayerController* USlitherPlayerUIComponent::GetLocalPlayerController() const
{
    AActor* OwnerActor = GetOwner();

    ASlitherPlayerController* PlayerController
        = Cast<ASlitherPlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return nullptr;
    }

    if (const ENetMode NetMode
        = PlayerController->GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return nullptr;
    }

    const bool LocalController
        = PlayerController->IsLocalController();

    if (!LocalController)
    {
        return nullptr;
    }

    return PlayerController;
}

bool USlitherPlayerUIComponent::CanRequestStartGame() const
{
    if (StartRequestPending || SessionActive)
    {
        return false;
    }

    ASlitherPlayerController* PlayerController
        = GetLocalPlayerController();

    if (!PlayerController)
    {
        return false;
    }

    const USlitherPlayerServerComponent* ServerComponent
        = PlayerController->GetPlayerServerComponent();

    if (!ServerComponent)
    {
        return false;
    }

    if (const APawn* CurrentPawn
        = PlayerController->GetPawn())
    {
        return false;
    }

    const ASlitherPlayerState* PlayerState
        = PlayerController->GetPlayerState<ASlitherPlayerState>();

    if (!PlayerState)
    {
        return false;
    }

    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return false;
    }

    const ASlitherGameState* GameState
        = ThisWorld->GetGameState<ASlitherGameState>();

    if (!GameState)
    {
        return false;
    }

    if (const bool WorldReady
            = GameState->IsWorldReady();
        !WorldReady)
    {
        return false;
    }

    const FSlitherPlayerLifeData& LifeData
        = PlayerState->GetLifeData();

    if (LifeData.LifeId != CurrentLifeId)
    {
        return false;
    }

    if (NewGame)
    {
        return CurrentLifeId == 0
            && LifeData.State == ESlitherLifeState::ReadyToStart;
    }

    if (LifeData.State != ESlitherLifeState::Dead
        || RespawnAvailableServerTime < 0.0)
    {
        return false;
    }

    const double ServerTime
        = GameState->GetServerWorldTimeSeconds();

    return ServerTime >= RespawnAvailableServerTime;
}

USlitherPlayerUIComponent::USlitherPlayerUIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void USlitherPlayerUIComponent::UpdateLocalPresentation()
{
    const ASlitherPlayerController* PlayerController
        = GetLocalPlayerController();

    if (!PlayerController)
    {
        return;
    }

    AHUD* ControllerHud = PlayerController->GetHUD();

    ASlitherHUD* SlitherHud = Cast<ASlitherHUD>(ControllerHud);

    if (!SlitherHud)
    {
        return;
    }

    if (SessionActive)
    {
        APawn* CurrentPawn = PlayerController->GetPawn();

        const ASlitherWormPawn* WormPawn
            = Cast<ASlitherWormPawn>(CurrentPawn);

        const ASlitherPlayerState* PlayerState
            = PlayerController->GetPlayerState<ASlitherPlayerState>();

        if (!WormPawn || !PlayerState)
        {
            return;
        }

        const bool BeingDestroyed
            = WormPawn->IsActorBeingDestroyed();

        const bool Alive = PlayerState->IsAlive();

        const uint32 PawnLifeId = WormPawn->GetLifeId();

        const uint32 PlayerLifeId = PlayerState->GetLifeId();

        const AController* PawnController
            = WormPawn->GetController();

        if (BeingDestroyed || !Alive
            || PawnLifeId != CurrentLifeId
            || PlayerLifeId != CurrentLifeId
            || PawnController != PlayerController)
        {
            return;
        }

        SlitherHud->ShowPlayerHud();
        return;
    }

    if (const bool MenuShown
            = SlitherHud->ShowRestartMenu();
        !MenuShown || ReadyEventSent)
    {
        return;
    }

    if (const bool Ready
            = CanRequestStartGame();
        !Ready)
    {
        return;
    }

    ReadyEventSent = true;

    OnReadyToRequestRestart.Broadcast();
}

void USlitherPlayerUIComponent::HandleSessionUpdate(
    const uint32 InLifeId,
    const int32 InKills,
    const int32 InMass,
    const int32 InTotalKillScore,
    const int32 InTotalMassScore,
    const int32 InSeconds,
    const bool InSessionActive,
    const double InRespawnAvailableServerTime)
{
    const ASlitherPlayerController* PlayerController
        = GetLocalPlayerController();

    if (!PlayerController || InLifeId == 0)
    {
        return;
    }

    if (InLifeId != CurrentLifeId)
    {
        if (!InSessionActive || SessionActive)
        {
            return;
        }

        CurrentLifeId = InLifeId;

        CurrentKills = 0;
        CurrentMass = 0;
        CurrentSeconds = 0;

        LastSessionSeconds = 0;

        RespawnAvailableServerTime = -1.0;

        NewGame = false;
        SessionActive = true;
        StartRequestPending = false;
        ReadyEventSent = false;
    }

    if (!SessionActive)
    {
        return;
    }

    if (CurrentKills != InKills)
    {
        CurrentKills = InKills;
        OnKill.Broadcast(CurrentKills);
    }

    if (CurrentMass != InMass)
    {
        CurrentMass = InMass;
        OnMassIncreased.Broadcast(CurrentMass);
    }

    if (CurrentSeconds != InSeconds)
    {
        CurrentSeconds = InSeconds;
        OnTimeChanged.Broadcast(CurrentSeconds);
    }

    if (!InSessionActive)
    {
        SetSessionResults(InTotalKillScore, InTotalMassScore, InSeconds);

        SessionActive = false;
        StartRequestPending = false;
        ReadyEventSent = false;

        RespawnAvailableServerTime
            = InRespawnAvailableServerTime;
    }

    UpdateLocalPresentation();
}

void USlitherPlayerUIComponent::HandleStartGameRequestResult(
    const uint32 ExpectedLifeId,
    const bool Accepted)
{
    if (ExpectedLifeId != CurrentLifeId
        || SessionActive
        || !StartRequestPending)
    {
        return;
    }

    if (!Accepted)
    {
        StartRequestPending = false;
    }
}

void USlitherPlayerUIComponent::SetSessionResults(
    const int32 InResultKills,
    const int32 InResultMass,
    const int32 InResultSeconds)
{
    LastSessionKills = InResultKills;
    LastSessionMass = InResultMass;
    LastSessionSeconds = InResultSeconds;
}

bool USlitherPlayerUIComponent::IsNewGame() const
{
    return NewGame;
}

void USlitherPlayerUIComponent::GetSessionResults(
    int32& FinalKills,
    int32& FinalMass,
    int32& FinalSeconds) const
{
    FinalKills = LastSessionKills;
    FinalMass = LastSessionMass;
    FinalSeconds = LastSessionSeconds;
}

void USlitherPlayerUIComponent::StartGame()
{
    if (const bool Ready
            = CanRequestStartGame();
        !Ready)
    {
        return;
    }

    const ASlitherPlayerController* PlayerController
        = GetLocalPlayerController();

    if (!PlayerController)
    {
        return;
    }

    USlitherPlayerServerComponent* ServerComponent
        = PlayerController->GetPlayerServerComponent();

    if (!ServerComponent)
    {
        return;
    }

    StartRequestPending = true;

    ServerComponent->ServerRequestStartGame(CurrentLifeId);
}
