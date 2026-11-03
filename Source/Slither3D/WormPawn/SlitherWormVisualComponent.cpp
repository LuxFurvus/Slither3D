
#include "SlitherWormVisualComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineGlobals.h"

#include "SlitherBodyComponent.h"
#include "Slither3D/SlitherGameState.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherWormVisualComponent::USlitherWormVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    SetIsReplicatedByDefault(false);
}

void USlitherWormVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    const AActor* OwnerActor = GetOwner();
    check(OwnerActor);
    const ENetMode NetMode = OwnerActor->GetNetMode();

    if (NetMode == NM_DedicatedServer)
    {
        SetComponentTickEnabled(false);
    }
}

void USlitherWormVisualComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ResetForLife(0);

    Super::EndPlay(EndPlayReason);
}

void USlitherWormVisualComponent::TickComponent(
    const float DeltaSeconds,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

    UpdateVisualState();
}

void USlitherWormVisualComponent::InitializeVisualComponents(
    USceneComponent* InWormRoot,
    UStaticMeshComponent* InHeadMesh,
    UInstancedStaticMeshComponent* InBodyMesh,
    USlitherBodyComponent* InWormBodyComponent)
{
    check(InWormRoot);
    check(InHeadMesh);
    check(InBodyMesh);
    check(InWormBodyComponent);

    WormRoot = InWormRoot;
    HeadMesh = InHeadMesh;
    BodyMesh = InBodyMesh;
    WormBodyComponent = InWormBodyComponent;
}

void USlitherWormVisualComponent::InitializeVisualState() const
{
    const AActor* OwnerActor = GetOwner();
    check(OwnerActor);
    const ENetMode NetMode = OwnerActor->GetNetMode();

    if (NetMode == NM_DedicatedServer)
    {
        return;
    }

    const FVector HeadLocation = OwnerActor->GetActorLocation();
    const FVector HeadDirection = OwnerActor->GetActorForwardVector();
    ApplyVisualTransform(HeadLocation, HeadDirection);
}

void USlitherWormVisualComponent::ApplyVisualTransform(
    const FVector& HeadLocation,
    const FVector& HeadDirection) const
{
    const FRotator HeadRotation = HeadDirection.Rotation();
    WormRoot->SetWorldLocationAndRotation(
        HeadLocation, HeadRotation, false, nullptr, ETeleportType::None);
}

FVector USlitherWormVisualComponent::GetVisualHeadLocation() const
{
    const UStaticMeshComponent* HeadMeshPointer
        = HeadMesh.Get();
    check(HeadMeshPointer);

    const FVector VisualHeadLocation
        = HeadMeshPointer->GetComponentLocation();

    return VisualHeadLocation;
}

void USlitherWormVisualComponent::ResetForLife(
    const uint32 LifeId)
{
    if (BufferedLifeId != LifeId || LifeId == 0)
    {
        SnapshotBuffer.Reset();
        PendingBodyTransforms.Reset();
        HasAcceptedSnapshot = false;
        HasRenderedState = false;
        LastRenderedHeadDistance = 0.0;
        LastRenderedBodyLength = 0.0;
        BufferedLifeId = LifeId;
        LastVisualUpdateFrame = MAX_uint64;
        LastRenderTime = -1.0;

        const AActor* OwnerActor = GetOwner();
        check(OwnerActor);
        const ENetMode NetMode = OwnerActor->GetNetMode();

        if (NetMode != NM_DedicatedServer)
        {
            if (HeadMesh)
            {
                HeadMesh->SetVisibility(false);
            }

            if (BodyMesh)
            {
                BodyMesh->SetVisibility(false);
                BodyMesh->ClearInstances();
            }
        }
    }
}

bool USlitherWormVisualComponent::TryBufferSnapshot(
    const FSlitherWormSnapshot& Snapshot)
{
    ResetForLife(Snapshot.LifeId);

    if (HasAcceptedSnapshot)
    {
        const uint32 FrameDelta =
            Snapshot.SimulationFrame - LastAcceptedSnapshot.SimulationFrame;
        const double LengthGrowth =
            Snapshot.BodyLength - LastAcceptedSnapshot.BodyLength;
        const double HeadMovement =
            Snapshot.HeadPathDistance - LastAcceptedSnapshot.HeadPathDistance;

        constexpr double GrowthTolerance = UE_KINDA_SMALL_NUMBER;

        const bool IsSnapshotWrong
            = FrameDelta == 0
            || FrameDelta > MaxForwardSequenceDelta
            || Snapshot.ServerTime <= LastAcceptedSnapshot.ServerTime
            || Snapshot.HeadPathDistance < LastAcceptedSnapshot.HeadPathDistance
            || LengthGrowth < 0.0
            || LengthGrowth > HeadMovement + GrowthTolerance
            || Snapshot.RequiredPathPointSequence
                < LastAcceptedSnapshot.RequiredPathPointSequence;

        if (IsSnapshotWrong)
        {
            return false;
        }
    }

    LastAcceptedSnapshot = Snapshot;
    HasAcceptedSnapshot = true;
    SnapshotBuffer.Emplace(Snapshot);

    const int32 NewCount = SnapshotBuffer.Num();
    const int32 ExcessCount
        = NewCount - FSlitherSettingsNetwork::MaxSnapshotCount;

    if (ExcessCount > 0)
    {
        SnapshotBuffer.RemoveAt(0, ExcessCount, EAllowShrinking::No);
    }

    DiscardObsoleteSnapshots();
    TrimBodyHistory();

    return true;
}

void USlitherWormVisualComponent::DiscardObsoleteSnapshots()
{
    if (!WormBodyComponent)
    {
        return;
    }

    int32 SnapshotIndex = 0;
    int32 SnapshotCount = SnapshotBuffer.Num();

    while (SnapshotIndex < SnapshotCount)
    {
        const bool SnapshotIsObsolete =
            WormBodyComponent->IsSnapshotObsolete(
                SnapshotBuffer[SnapshotIndex]);

        if (SnapshotIsObsolete)
        {
            SnapshotBuffer.RemoveAt(
                SnapshotIndex, 1, EAllowShrinking::No);
            --SnapshotCount;
        }
        else
        {
            ++SnapshotIndex;
        }
    }
}

void USlitherWormVisualComponent::TrimBodyHistory() const
{
    if (!WormBodyComponent)
    {
        return;
    }

    double MinimumDistance = MAX_dbl;

    if (HasRenderedState)
    {
        MinimumDistance
            = LastRenderedHeadDistance - LastRenderedBodyLength;
    }

    for (const FSlitherWormSnapshot& Snapshot : SnapshotBuffer)
    {
        const double TailDistance
            = Snapshot.HeadPathDistance - Snapshot.BodyLength;

        MinimumDistance
            = FMath::Min(MinimumDistance, TailDistance);
    }

    if (MinimumDistance < MAX_dbl)
    {
        WormBodyComponent->TrimCachedPath(MinimumDistance);
    }
}

bool USlitherWormVisualComponent::ApplyVisualState(
    const FVector& HeadLocation,
    const FVector& HeadDirection) const
{
    const int32 InstanceCount = BodyMesh->GetInstanceCount();
    const int32 SegmentCount = PendingBodyTransforms.Num();

    if (InstanceCount > SegmentCount)
    {
        return false;
    }

    const FTransform PreviousVisualTransform
        = WormRoot->GetComponentTransform();

    ApplyVisualTransform(HeadLocation, HeadDirection);

    for (int32 SegmentIndex = InstanceCount; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        const FTransform& SegmentTransform
            = PendingBodyTransforms[SegmentIndex];

        BodyMesh->AddInstance(SegmentTransform, true);
    }

    const bool TransformsUpdated
        = BodyMesh->BatchUpdateInstancesTransforms(
            0, PendingBodyTransforms, true, true, true);

    if (!TransformsUpdated)
    {
        WormRoot->SetWorldTransform(
            PreviousVisualTransform, false, nullptr, ETeleportType::None);
        return false;
    }

    HeadMesh->SetVisibility(true);
    BodyMesh->SetVisibility(true);

    return true;
}

void USlitherWormVisualComponent::UpdateVisualState()
{
    const AActor* OwnerActor = GetOwner();
    check(OwnerActor);

    const ENetMode NetMode = OwnerActor->GetNetMode();

    if (NetMode == NM_DedicatedServer
        || LastVisualUpdateFrame == GFrameCounter)
    {
        return;
    }

    LastVisualUpdateFrame = GFrameCounter;
    DiscardObsoleteSnapshots();

    const int32 SnapshotCount = SnapshotBuffer.Num();

    if (SnapshotCount == 0 || !WormBodyComponent || !BodyMesh)
    {
        return;
    }

    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return;
    }

    const ASlitherGameState* SlitherGameState =
        ThisWorld->GetGameState<ASlitherGameState>();

    if (!SlitherGameState)
    {
        return;
    }

    const double ServerNow = SlitherGameState->GetServerWorldTimeSeconds();
    const double TargetTime =
        ServerNow - FSlitherSettingsNetwork::ClientInterpolationDelay;
    const double RenderTime = FMath::Max(TargetTime, LastRenderTime);
    int32 FirstIndex = 0;

    while (FirstIndex + 1 < SnapshotCount
        && SnapshotBuffer[FirstIndex + 1].ServerTime <= RenderTime)
    {
        ++FirstIndex;
    }

    const int32 SecondIndex = FMath::Min(FirstIndex + 1, SnapshotCount - 1);
    const FSlitherWormSnapshot& FirstSnapshot = SnapshotBuffer[FirstIndex];
    const FSlitherWormSnapshot& SecondSnapshot = SnapshotBuffer[SecondIndex];
    const double Interval = SecondSnapshot.ServerTime - FirstSnapshot.ServerTime;
    double Alpha = 0.0;

    if (Interval > 0.0)
    {
        const double TimeFraction =
            (RenderTime - FirstSnapshot.ServerTime) / Interval;
        Alpha = FMath::Clamp(TimeFraction, 0.0, 1.0);
    }

    const FVector HeadLocation = FMath::Lerp(
        FirstSnapshot.HeadLocation, SecondSnapshot.HeadLocation, Alpha);
    const FRotator FirstRotation = FirstSnapshot.HeadDirection.Rotation();
    const FRotator SecondRotation = SecondSnapshot.HeadDirection.Rotation();
    const double YawDelta = FMath::FindDeltaAngleDegrees(
        FirstRotation.Yaw, SecondRotation.Yaw);
    const double HeadYaw = FirstRotation.Yaw + YawDelta * Alpha;
    const FRotator HeadRotation(0.0, HeadYaw, 0.0);
    const FVector HeadDirection = HeadRotation.Vector();
    const double RenderPathDistance = FMath::Lerp(
        FirstSnapshot.HeadPathDistance, SecondSnapshot.HeadPathDistance, Alpha);
    const double RenderBodyLength = FMath::Lerp(
        FirstSnapshot.BodyLength, SecondSnapshot.BodyLength, Alpha);
    const uint64 RequiredPathPointSequence = Alpha > 0.0
        ? SecondSnapshot.RequiredPathPointSequence
        : FirstSnapshot.RequiredPathPointSequence;

    const bool BodyIsReady = WormBodyComponent->TryBuildVisualBody(
        HeadLocation, HeadDirection, RenderPathDistance, RenderBodyLength,
        RequiredPathPointSequence, PendingBodyTransforms);

    if (!BodyIsReady)
    {
        return;
    }

    const bool VisualStateApplied = ApplyVisualState(
        HeadLocation, HeadDirection);

    if (!VisualStateApplied)
    {
        return;
    }

    LastRenderTime = FMath::Lerp(
        FirstSnapshot.ServerTime, SecondSnapshot.ServerTime, Alpha);
    LastRenderedHeadDistance = RenderPathDistance;
    LastRenderedBodyLength = RenderBodyLength;
    HasRenderedState = true;

    if (FirstIndex > 0)
    {
        SnapshotBuffer.RemoveAt(0, FirstIndex, EAllowShrinking::No);
    }

    TrimBodyHistory();
}

