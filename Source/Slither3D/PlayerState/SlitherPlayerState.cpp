
#include "SlitherPlayerState.h"

#include "SlitherPlayerLifeComponent.h"
#include "SlitherPlayerSessionStatsComponent.h"
#include "Slither3D/SlitherGameState.h"
#include "Slither3D/PlayerController/SlitherPlayerController.h"
#include "Slither3D/PlayerController/SlitherPlayerServerComponent.h"


ASlitherPlayerState::ASlitherPlayerState()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    LifeComponent
        = CreateDefaultSubobject<USlitherPlayerLifeComponent>(
            TEXT("LifeComponent"));
    check(LifeComponent);

    SessionStatsComponent
        = CreateDefaultSubobject<USlitherPlayerSessionStatsComponent>(
            TEXT("SessionStatsComponent"));
    check(SessionStatsComponent);
}

void ASlitherPlayerState::PublishSessionSeconds(
    const double ServerTime) const
{
    const double Duration
        = LifeComponent->GetLifeDuration(ServerTime);

    while (const bool Advanced
        = SessionStatsComponent->TryAdvancePublishedSeconds(Duration))
    {
        NotifySessionUpdate(true);
    }
}

void ASlitherPlayerState::NotifySessionUpdate(
    const bool SessionActive) const
{
    const bool Authority = HasAuthority();

    const FSlitherPlayerLifeData& CurrentLifeData
        = LifeComponent->GetLifeData();

    if (!Authority || CurrentLifeData.LifeId == 0)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();

    const ASlitherPlayerController* PlayerController
        = Cast<ASlitherPlayerController>(OwnerActor);

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

    const uint32 LifeId = CurrentLifeData.LifeId;
    const int32 Kills = CurrentLifeData.Kills;

    const int32 DisplayMass
        = SessionStatsComponent->CalculateDisplayMass(CurrentLifeData);
    const int32 TotalKillScore
        = SessionStatsComponent->GetTotalKillScore();
    const int32 TotalMassScore
        = SessionStatsComponent->GetTotalMassScore();
    const int32 PublishedSeconds
        = SessionStatsComponent->GetPublishedSeconds();
    const double RespawnAvailableServerTime
        = CurrentLifeData.RespawnAvailableServerTime;

    ServerComponent->ClientSessionUpdate(
        LifeId,
        Kills,
        DisplayMass,
        TotalKillScore,
        TotalMassScore,
        PublishedSeconds,
        SessionActive,
        RespawnAvailableServerTime);
}

void ASlitherPlayerState::Tick(
    const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const bool Authority = HasAuthority();

    const bool Alive = LifeComponent->IsAlive();

    if (!Authority || !Alive)
    {
        return;
    }

    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return;
    }

    const ASlitherGameState* GameState
        = ThisWorld->GetGameState<ASlitherGameState>();

    if (!GameState)
    {
        return;
    }

    const double ServerTime
        = GameState->GetServerWorldTimeSeconds();

    PublishSessionSeconds(ServerTime);
}

uint32 ASlitherPlayerState::GetLifeId() const
{
    return LifeComponent->GetLifeId();
}

int32 ASlitherPlayerState::GetMass() const
{
    return LifeComponent->GetMass();
}

bool ASlitherPlayerState::IsAlive() const
{
    return LifeComponent->IsAlive();
}

int32 ASlitherPlayerState::GetKills() const
{
    return LifeComponent->GetKills();
}

bool ASlitherPlayerState::TryEndLife(
    const uint32 ExpectedLifeId) const
{
    return LifeComponent->TryEndLife(ExpectedLifeId);
}

void ASlitherPlayerState::AddKill(
    const uint32 ExpectedLifeId)
{
    const bool Awarded = LifeComponent->AddKill(ExpectedLifeId);

    if (!Awarded)
    {
        return;
    }

    ForceNetUpdate();
    NotifySessionUpdate(true);
}

int32 ASlitherPlayerState::AddMass(
    const int32 Nutrition)
{
    const int32 AcceptedMass = LifeComponent->AddMass(Nutrition);

    if (AcceptedMass <= 0)
    {
        return 0;
    }

    ForceNetUpdate();
    NotifySessionUpdate(true);

    return AcceptedMass;
}

const FSlitherPlayerLifeData& ASlitherPlayerState::GetLifeData() const
{
    return LifeComponent->GetLifeData();
}

bool ASlitherPlayerState::IsWaitingForSpawn() const
{
    return LifeComponent->IsWaitingForSpawn();
}

uint32 ASlitherPlayerState::CalculateNextLifeId() const
{
    return LifeComponent->CalculateNextLifeId();
}

bool ASlitherPlayerState::TryBeginNextLife(
    const uint32 ExpectedLifeId,
    const double SpawnServerTime)
{
    const bool BeganLife = LifeComponent->TryBeginNextLife(
        ExpectedLifeId, SpawnServerTime);

    if (!BeganLife)
    {
        return false;
    }

    SessionStatsComponent->ResetForNextLife();

    SetActorTickEnabled(true);

    ForceNetUpdate();

    NotifySessionUpdate(true);

    return true;
}

bool ASlitherPlayerState::TryQueueRespawn(
    const uint32 ExpectedLifeId,
    const double ServerTime)
{
    const bool Queued = LifeComponent->TryQueueRespawn(
        ExpectedLifeId, ServerTime);

    if (!Queued)
    {
        return false;
    }

    ForceNetUpdate();
    return true;
}

void ASlitherPlayerState::FinalizeLifeDeath(
    const uint32 ExpectedLifeId,
    const double ServerTime)
{
    const bool Finalized = LifeComponent->FinalizeLifeDeath(
        ExpectedLifeId, ServerTime);

    if (!Finalized)
    {
        return;
    }

    const FSlitherPlayerLifeData& CompletedLife
        = LifeComponent->GetLifeData();
    SessionStatsComponent->RecordCompletedLife(CompletedLife);

    PublishSessionSeconds(ServerTime);

    SetActorTickEnabled(false);

    ForceNetUpdate();

    NotifySessionUpdate(false);
}

double ASlitherPlayerState::GetLifeDuration(
    const double ServerTime) const
{
    return LifeComponent->GetLifeDuration(ServerTime);
}

double ASlitherPlayerState::GetRespawnRemaining(
    const double ServerTime) const
{
    return LifeComponent->GetRespawnRemaining(ServerTime);
}

bool ASlitherPlayerState::TryQueueFirstSpawn()
{
    if (const bool Queued
            = LifeComponent->TryQueueFirstSpawn();
        !Queued)
    {
        return false;
    }

    ForceNetUpdate();

    return true;
}

