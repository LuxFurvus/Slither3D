
#include "SlitherPlayerLifeComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"


USlitherPlayerLifeComponent::USlitherPlayerLifeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

bool USlitherPlayerLifeComponent::HasOwnerAuthority() const
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return false;
    }

    return OwnerActor->HasAuthority();
}

uint32 USlitherPlayerLifeComponent::GetLifeId() const
{
    return LifeData.LifeId;
}

int32 USlitherPlayerLifeComponent::GetMass() const
{
    return LifeData.Mass;
}

bool USlitherPlayerLifeComponent::IsAlive() const
{
    return LifeData.State == ESlitherLifeState::Alive;
}

int32 USlitherPlayerLifeComponent::GetKills() const
{
    return LifeData.Kills;
}

bool USlitherPlayerLifeComponent::TryEndLife(
    const uint32 ExpectedLifeId)
{
    const bool HasServerAuthority
        = HasOwnerAuthority();

    if (!HasServerAuthority
        || LifeData.State != ESlitherLifeState::Alive
        || ExpectedLifeId == 0
        || LifeData.LifeId != ExpectedLifeId)
    {
        return false;
    }

    LifeData.State = ESlitherLifeState::Dead;

    return true;
}

bool USlitherPlayerLifeComponent::FinalizeLifeDeath(
    const uint32 ExpectedLifeId,
    const double ServerTime)
{
    const bool HasServerAuthority = HasOwnerAuthority();

    const bool FinalizeLifeFailed
        = !HasServerAuthority
        || ExpectedLifeId == 0
        || LifeData.LifeId != ExpectedLifeId
        || LifeData.State != ESlitherLifeState::Dead
        || LifeData.DeathServerTime >= 0.0
        || ServerTime < LifeData.SpawnServerTime;

    if (FinalizeLifeFailed)
    {
        return false;
    }

    LifeData.DeathServerTime = ServerTime;

    LifeData.RespawnAvailableServerTime
        = ServerTime + FSlitherSettingsRespawn::RespawnDelay;

    return true;
}

bool USlitherPlayerLifeComponent::AddKill(
    const uint32 ExpectedLifeId)
{
    const bool HasServerAuthority = HasOwnerAuthority();

    const bool CanAwardKill
        = LifeData.State == ESlitherLifeState::Alive
        || (LifeData.State == ESlitherLifeState::Dead
            && LifeData.DeathServerTime < 0.0);

    const bool IncreaseKillCountFailed
        = !HasServerAuthority
        || !CanAwardKill
        || ExpectedLifeId == 0
        || LifeData.LifeId != ExpectedLifeId
        || LifeData.Kills == MAX_int32;

    if (IncreaseKillCountFailed)
    {
        return false;
    }

    ++LifeData.Kills;

    return true;
}

int32 USlitherPlayerLifeComponent::AddMass(
    const int32 Nutrition)
{
    const bool HasServerAuthority = HasOwnerAuthority();

    if (!HasServerAuthority
        || LifeData.State != ESlitherLifeState::Alive
        || Nutrition <= 0)
    {
        return 0;
    }

    const int32 RemainingMass = MAX_int32 - LifeData.Mass;
    const int32 AcceptedMass = FMath::Min(Nutrition, RemainingMass);

    if (AcceptedMass <= 0)
    {
        return 0;
    }

    LifeData.Mass += AcceptedMass;

    return AcceptedMass;
}

void USlitherPlayerLifeComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USlitherPlayerLifeComponent, LifeData);
}

const FSlitherPlayerLifeData& USlitherPlayerLifeComponent::GetLifeData() const
{
    return LifeData;
}

bool USlitherPlayerLifeComponent::IsWaitingForSpawn() const
{
    return LifeData.State == ESlitherLifeState::WaitingForSpawn;
}

bool USlitherPlayerLifeComponent::TryQueueRespawn(
    const uint32 ExpectedLifeId,
    const double ServerTime)
{
    const bool HasServerAuthority = HasOwnerAuthority();

    if (!HasServerAuthority || ExpectedLifeId == 0
        || LifeData.LifeId != ExpectedLifeId
        || LifeData.State != ESlitherLifeState::Dead
        || LifeData.DeathServerTime < 0.0
        || LifeData.RespawnAvailableServerTime < 0.0
        || ServerTime < LifeData.RespawnAvailableServerTime)
    {
        return false;
    }

    LifeData.State = ESlitherLifeState::WaitingForSpawn;
    return true;
}

uint32 USlitherPlayerLifeComponent::CalculateNextLifeId() const
{
    uint32 NextLifeId = LifeData.LifeId + 1;

    if (NextLifeId == 0)
    {
        ++NextLifeId;
    }

    return NextLifeId;
}

bool USlitherPlayerLifeComponent::TryBeginNextLife(
    const uint32 ExpectedLifeId,
    const double SpawnServerTime)
{
    const bool HasServerAuthority
        = HasOwnerAuthority();

    if (!HasServerAuthority
        || LifeData.LifeId != ExpectedLifeId
        || LifeData.State != ESlitherLifeState::WaitingForSpawn
        || SpawnServerTime < 0.0)
    {
        return false;
    }

    FSlitherPlayerLifeData NewLifeData;
    NewLifeData.LifeId = CalculateNextLifeId();
    NewLifeData.State = ESlitherLifeState::Alive;
    NewLifeData.SpawnServerTime = SpawnServerTime;
    LifeData = NewLifeData;

    return true;
}

double USlitherPlayerLifeComponent::GetLifeDuration(
    const double ServerTime) const
{
    if (LifeData.LifeId == 0
        || LifeData.SpawnServerTime < 0.0)
    {
        return 0.0;
    }

    const double EndTime
        = LifeData.State == ESlitherLifeState::Alive
        ? ServerTime : LifeData.DeathServerTime;

    const double Duration
        = EndTime - LifeData.SpawnServerTime;
    const double NonnegativeDuration
        = FMath::Max(Duration, 0.0);

    return NonnegativeDuration;
}

double USlitherPlayerLifeComponent::GetRespawnRemaining(
    const double ServerTime) const
{
    if (LifeData.LifeId == 0
        || LifeData.State == ESlitherLifeState::Alive
        || LifeData.RespawnAvailableServerTime < 0.0)
    {
        return 0.0;
    }

    const double Remaining
        = LifeData.RespawnAvailableServerTime - ServerTime;

    const double NonnegativeRemaining
        = FMath::Max(Remaining, 0.0);

    return NonnegativeRemaining;
}

bool USlitherPlayerLifeComponent::TryQueueFirstSpawn()
{
    const bool HasServerAuthority
        = HasOwnerAuthority();

    if (!HasServerAuthority
        || LifeData.LifeId != 0
        || LifeData.State != ESlitherLifeState::ReadyToStart)
    {
        return false;
    }

    LifeData.State
        = ESlitherLifeState::WaitingForSpawn;

    return true;
}
