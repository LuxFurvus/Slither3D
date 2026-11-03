
#include "SlitherWormReplicationComponent.h"

#include "Net/UnrealNetwork.h"

#include "SlitherWormPawn.h"
#include "SlitherBodyComponent.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherWormReplicationComponent::USlitherWormReplicationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

ASlitherWormPawn* USlitherWormReplicationComponent::GetWormPawn() const
{
    AActor* OwnerActor = GetOwner();
    ASlitherWormPawn* WormPawn
        = Cast<ASlitherWormPawn>(OwnerActor);
    check(WormPawn);

    return WormPawn;
}

void USlitherWormReplicationComponent::PublishSnapshot(
    const uint32 SimulationFrame,
    const double ServerTime,
    const float CurrentSpeed)
{
    ASlitherWormPawn* WormPawn = GetWormPawn();
    const uint32 LifeId = WormPawn->GetLifeId();

    const bool LifeActive = WormPawn->HasActiveLife();

    if (const bool HasServerAuthority = WormPawn->HasAuthority();
        !HasServerAuthority || !LifeActive || LifeId == 0)
    {
        return;
    }

    if (LatestSnapshot.LifeId == LifeId)
    {
        if (const uint32 FrameDelta = SimulationFrame - LatestSnapshot.SimulationFrame;
            FrameDelta == 0
            || FrameDelta > MaxForwardSequenceDelta
            || ServerTime <= LatestSnapshot.ServerTime)
        {
            return;
        }
    }

    const USlitherBodyComponent* BodyComponent = WormPawn->GetBodyComponent();
    check(BodyComponent);
    const uint64 RequiredPathPointSequence
        = BodyComponent->GetLastPathPointSequence();

    if (RequiredPathPointSequence == 0)
    {
        return;
    }

    LatestSnapshot.LifeId = LifeId;
    LatestSnapshot.SimulationFrame = SimulationFrame;
    LatestSnapshot.ServerTime = ServerTime;
    LatestSnapshot.HeadLocation = WormPawn->GetActorLocation();
    LatestSnapshot.HeadDirection = WormPawn->GetActorForwardVector();
    LatestSnapshot.Speed = CurrentSpeed;
    LatestSnapshot.HeadPathDistance = BodyComponent->GetHeadPathDistance();
    LatestSnapshot.BodyLength = BodyComponent->GetBodyLength();
    LatestSnapshot.RequiredPathPointSequence = RequiredPathPointSequence;

    if (const ENetMode NetMode
            = WormPawn->GetNetMode();
        NetMode != NM_DedicatedServer)
    {
        ReceiveSnapshot(LatestSnapshot);
    }

    WormPawn->ForceNetUpdate();
}

void USlitherWormReplicationComponent::ReceiveSnapshot(
    const FSlitherWormSnapshot& Snapshot) const
{
    ASlitherWormPawn* WormPawn = GetWormPawn();

    const uint32 LifeId = WormPawn->GetLifeId();

    if (LifeId == 0 || Snapshot.LifeId != LifeId)
    {
        return;
    }

    const bool DirectionIsNormalized
        = Snapshot.HeadDirection.IsNormalized();
    const double AbsoluteZ
        = FMath::Abs(Snapshot.HeadDirection.Z);

    const bool SomeValuesInvalid
        = !DirectionIsNormalized
        || Snapshot.BodyLength < FSlitherSettingsGrowth::InitialBodyLength
        || Snapshot.BodyLength > FSlitherSettingsGrowth::MaxBodyLength
        || Snapshot.HeadPathDistance < 0.0
        || Snapshot.RequiredPathPointSequence == 0
        || AbsoluteZ > UE_SMALL_NUMBER || Snapshot.Speed < 0.0f;

    if (SomeValuesInvalid)
    {
        return;
    }

    const bool SnapshotAccepted
        = WormPawn->TryBufferVisualSnapshot(Snapshot);

    if (!SnapshotAccepted)
    {
        return;
    }

    if (const bool HasServerAuthority
            = WormPawn->HasAuthority();
        !HasServerAuthority)
    {
        const FRotator HeadRotation
            = Snapshot.HeadDirection.Rotation();

        WormPawn->SetActorLocationAndRotation(
            Snapshot.HeadLocation, HeadRotation,
            false, nullptr, ETeleportType::None);
    }
}

void USlitherWormReplicationComponent::OnRep_LatestSnapshot() const
{
    ProcessLatestSnapshot();
}

void USlitherWormReplicationComponent::ProcessLatestSnapshot() const
{
    const ASlitherWormPawn* WormPawn
        = GetWormPawn();

    if (const ENetMode NetMode
            = WormPawn->GetNetMode();
        NetMode != NM_DedicatedServer)
    {
        ReceiveSnapshot(LatestSnapshot);
    }
}

void USlitherWormReplicationComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USlitherWormReplicationComponent, LatestSnapshot);
}
