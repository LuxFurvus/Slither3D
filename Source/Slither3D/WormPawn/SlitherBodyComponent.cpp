
#include "SlitherBodyComponent.h"

#include "Net/UnrealNetwork.h"

#include "SlitherWormPawn.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherBodyComponent::USlitherBodyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);

    BodyLength = FSlitherSettingsGrowth::InitialBodyLength;
    NextSampleDistance = FSlitherSettingsBody::PathSampleDistance;
}

ASlitherWormPawn* USlitherBodyComponent::GetWormPawn() const
{
    AActor* OwnerActor = GetOwner();

    ASlitherWormPawn* WormPawn
        = Cast<ASlitherWormPawn>(OwnerActor);
    check(WormPawn);

    return WormPawn;
}

void USlitherBodyComponent::OnRegister()
{
    Super::OnRegister();

    ReplicatedPath.SetOwnerComponent(this);
    FinishPathUpdate();
}

void USlitherBodyComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ResetForLife(0);
    ReplicatedPath.SetOwnerComponent(nullptr);

    Super::EndPlay(EndPlayReason);
}

void USlitherBodyComponent::ResetForLife(
    const uint32 LifeId)
{
    if (ActiveLifeId == LifeId && LifeId > 0)
    {
        ImportReplicatedPath();
        return;
    }

    ActiveLifeId = LifeId;
    NextPointSequence = 1;
    WaitingForPointSequence = 0;
    HeadPathDistance = 0.0;
    NextSampleDistance = FSlitherSettingsBody::PathSampleDistance;
    BodyLength = FSlitherSettingsGrowth::InitialBodyLength;
    CachedPoints.Reset();
    ServerSegmentLocations.Reset();

    if (LifeId > 0)
    {
        ImportReplicatedPath();
    }
}

void USlitherBodyComponent::InitializeLife(
    const uint32 LifeId,
    const FVector& HeadLocation,
    const FVector& HeadDirection)
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();

    if (!HasServerAuthority || LifeId == 0 || ActiveLifeId == LifeId)
    {
        return;
    }

    ResetForLife(LifeId);
    ReplicatedPath.Reset();

    const double HistoryLength
        = BodyLength + FSlitherSettingsBody::HistoryReserveDistance;

    constexpr double SampleDistance
        = FSlitherSettingsBody::PathSampleDistance;

    const int32 InitialIntervalCount = FMath::CeilToInt(
        HistoryLength / SampleDistance);

    for (int32 PointIndex = 0;
        PointIndex <= InitialIntervalCount; ++PointIndex)
    {
        const double PointDistance =
            (PointIndex - InitialIntervalCount) * SampleDistance;
        const FVector PointLocation =
            HeadLocation + HeadDirection * PointDistance;

        AppendPathPoint(PointLocation, PointDistance);
    }

    UpdateServerSegments();
}

void USlitherBodyComponent::AppendPathPoint(
    const FVector& Location,
    const double PathDistance)
{
    FSlitherPathPoint Point;
    Point.LifeId = ActiveLifeId;
    Point.PointSequence = NextPointSequence;
    Point.Location = Location;
    Point.PathDistance = PathDistance;
    ++NextPointSequence;

    ReplicatedPath.AddPoint(Point);
    CachePathPoint(Point);
}

void USlitherBodyComponent::RecordHeadMovement(
    const FVector& PreviousHeadLocation,
    const FVector& CurrentHeadLocation)
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();
    const uint32 LifeId = WormPawn->GetLifeId();

    if (!HasServerAuthority || ActiveLifeId == 0 || ActiveLifeId != LifeId)
    {
        return;
    }

    const FVector MovementDelta =
        CurrentHeadLocation - PreviousHeadLocation;
    const double MovementDistance = MovementDelta.Size2D();

    if (MovementDistance > 0.0)
    {
        const double NewHeadPathDistance =
            HeadPathDistance + MovementDistance;

        while (NextSampleDistance <= NewHeadPathDistance)
        {
            const double SampleAlpha =
                (NextSampleDistance - HeadPathDistance)
                / MovementDistance;
            const FVector SampleLocation = FMath::Lerp(
                PreviousHeadLocation, CurrentHeadLocation, SampleAlpha);

            AppendPathPoint(SampleLocation, NextSampleDistance);

            NextSampleDistance +=
                FSlitherSettingsBody::PathSampleDistance;
        }

        HeadPathDistance = NewHeadPathDistance;
    }
}

void USlitherBodyComponent::RefreshServerSegments()
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();
    const bool LifeActive = WormPawn->HasActiveLife();
    const uint32 LifeId = WormPawn->GetLifeId();

    if (!HasServerAuthority || !LifeActive || ActiveLifeId != LifeId)
    {
        return;
    }

    UpdateServerSegments();
}

void USlitherBodyComponent::ApplyGrowth(
    const int32 Mass,
    const double MovedDistance)
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();
    const uint32 LifeId = WormPawn->GetLifeId();

    if (!HasServerAuthority || ActiveLifeId == 0 || ActiveLifeId != LifeId)
    {
        return;
    }

    const int32 LimitedMass = FMath::Clamp(
        Mass, FSlitherSettingsGrowth::InitialMass, FSlitherSettingsGrowth::MaxMass);
    const int32 AdditionalMass = LimitedMass - FSlitherSettingsGrowth::InitialMass;
    const double RequestedLength = FSlitherSettingsGrowth::InitialBodyLength
        + AdditionalMass * FSlitherSettingsGrowth::LengthPerMassUnit;
    const double TargetLength = FMath::Min(
        RequestedLength, FSlitherSettingsGrowth::MaxBodyLength);
    const double AvailableGrowth = FMath::Max(MovedDistance, 0.0);
    const double ReachableLength = BodyLength + AvailableGrowth;
    const double NewLength = FMath::Min(TargetLength, ReachableLength);
    BodyLength = FMath::Max(BodyLength, NewLength);

    const double MinimumDistance = HeadPathDistance - BodyLength
        - FSlitherSettingsBody::HistoryReserveDistance;
    ReplicatedPath.TrimBefore(
        MinimumDistance, FSlitherSettingsGrowth::MaxServerPathPointCount);
    UpdateServerSegments();
}

int32 USlitherBodyComponent::CalculateSegmentCount(const double Length)
{
    const double SegmentRatio = Length / FSlitherSettingsBody::SegmentSpacing;
    const int32 SegmentCount = FMath::CeilToInt(SegmentRatio);
    const int32 LimitedCount = FMath::Clamp(
        SegmentCount, 0, FSlitherSettingsGrowth::MaxElementCount - 1);
    return LimitedCount;
}

double USlitherBodyComponent::CalculateSegmentOffset(
    const int32 SegmentIndex,
    const double Length)
{
    const double FullOffset = (SegmentIndex + 1) * FSlitherSettingsBody::SegmentSpacing;
    const double Offset = FMath::Min(FullOffset, Length);
    return Offset;
}

double USlitherBodyComponent::GetHeadPathDistance() const
{
    return HeadPathDistance;
}

double USlitherBodyComponent::GetBodyLength() const
{
    return BodyLength;
}

uint64 USlitherBodyComponent::GetLastPathPointSequence() const
{
    return NextPointSequence - 1;
}

const TArray<FVector>& USlitherBodyComponent::GetServerSegmentLocations() const
{
    return ServerSegmentLocations;
}

int32 USlitherBodyComponent::GetServerPathPointCount() const
{
    const TArray<FSlitherPathPoint>& Points = ReplicatedPath.GetPoints();
    const int32 PointCount = Points.Num();

    return PointCount;
}

int32 USlitherBodyComponent::GetCachedPathPointCount() const
{
    const int32 PointCount = CachedPoints.Num();

    return PointCount;
}

uint64 USlitherBodyComponent::GetWaitingForPointSequence() const
{
    return WaitingForPointSequence;
}

bool USlitherBodyComponent::ComparePathPoints(
    const FSlitherPathPoint& FirstPoint,
    const FSlitherPathPoint& SecondPoint)
{
    return FirstPoint.PointSequence < SecondPoint.PointSequence;
}

void USlitherBodyComponent::CachePathPoint(
    const FSlitherPathPoint& Point)
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const ENetMode NetMode = WormPawn->GetNetMode();

    if (NetMode == NM_DedicatedServer || ActiveLifeId == 0
        || Point.LifeId != ActiveLifeId || Point.PointSequence == 0)
    {
        return;
    }

    for (const FSlitherPathPoint& CachedPoint : CachedPoints)
    {
        if (CachedPoint.PointSequence == Point.PointSequence)
        {
            return;
        }
    }

    CachedPoints.Emplace(Point);
    CachedPoints.Sort(&USlitherBodyComponent::ComparePathPoints);

    const int32 PointCount = CachedPoints.Num();
    const int32 ExcessCount =
        PointCount - FSlitherSettingsBody::MaxClientPathPointCount;

    if (ExcessCount > 0)
    {
        CachedPoints.RemoveAt(0, ExcessCount, EAllowShrinking::No);
    }
}

void USlitherBodyComponent::ImportReplicatedPath()
{
    const TArray<FSlitherPathPoint>& Points = ReplicatedPath.GetPoints();

    for (const FSlitherPathPoint& Point : Points)
    {
        CachePathPoint(Point);
    }
}

void USlitherBodyComponent::FinishPathUpdate()
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const uint32 LifeId = WormPawn->GetLifeId();

    if (ActiveLifeId != LifeId)
    {
        ResetForLife(LifeId);
        return;
    }

    ImportReplicatedPath();
}

void USlitherBodyComponent::TrimCachedPath(
    const double MinimumDistance)
{
    const int32 PointCount = CachedPoints.Num();
    int32 RemoveCount = 0;

    while (RemoveCount + 1 < PointCount
        && CachedPoints[RemoveCount + 1].PathDistance <= MinimumDistance)
    {
        ++RemoveCount;
    }

    if (RemoveCount > 0)
    {
        CachedPoints.RemoveAt(0, RemoveCount, EAllowShrinking::No);
    }
}

bool USlitherBodyComponent::HasVisualPathRange(
    const double RenderPathDistance,
    const double RenderBodyLength,
    const uint64 RequiredPathPointSequence) const
{
    const double TailPathDistance = RenderPathDistance - RenderBodyLength;
    const int32 PointCount = CachedPoints.Num();
    int32 RequiredIndex = INDEX_NONE;

    for (int32 PointIndex = 0; PointIndex < PointCount; ++PointIndex)
    {
        if (CachedPoints[PointIndex].PointSequence == RequiredPathPointSequence)
        {
            RequiredIndex = PointIndex;
            break;
        }
    }

    if (RequiredIndex == INDEX_NONE
        || CachedPoints[0].PathDistance > TailPathDistance)
    {
        return false;
    }

    const double RequiredPointDistance = CachedPoints[RequiredIndex].PathDistance;
    const double UnsampledDistance = RenderPathDistance - RequiredPointDistance;

    if (TailPathDistance > RequiredPointDistance
        || UnsampledDistance > FSlitherSettingsBody::PathSampleDistance
            + UE_SMALL_NUMBER)
    {
        return false;
    }

    int32 FirstIndex = 0;

    while (FirstIndex < RequiredIndex
        && CachedPoints[FirstIndex + 1].PathDistance <= TailPathDistance)
    {
        ++FirstIndex;
    }

    for (int32 PointIndex = FirstIndex + 1;
        PointIndex <= RequiredIndex; ++PointIndex)
    {
        const FSlitherPathPoint& PreviousPoint = CachedPoints[PointIndex - 1];
        const FSlitherPathPoint& Point = CachedPoints[PointIndex];

        if (Point.PointSequence != PreviousPoint.PointSequence + 1
            || Point.PathDistance <= PreviousPoint.PathDistance)
        {
            return false;
        }
    }

    return true;
}

bool USlitherBodyComponent::IsSnapshotObsolete(
    const FSlitherWormSnapshot& Snapshot) const
{
    if (ActiveLifeId == 0)
    {
        return false;
    }

    if (Snapshot.LifeId != ActiveLifeId)
    {
        return true;
    }

    const int32 PointCount = CachedPoints.Num();

    if (PointCount == 0)
    {
        return false;
    }

    const FSlitherPathPoint& LastPoint = CachedPoints[PointCount - 1];

    if (Snapshot.RequiredPathPointSequence > LastPoint.PointSequence)
    {
        return false;
    }

    const double TailPathDistance = Snapshot.HeadPathDistance - Snapshot.BodyLength;

    if (TailPathDistance < CachedPoints[0].PathDistance)
    {
        return true;
    }

    const TArray<FSlitherPathPoint>& Points = ReplicatedPath.GetPoints();
    uint64 OldestReplicatedSequence = MAX_uint64;

    for (const FSlitherPathPoint& Point : Points)
    {
        if (Point.LifeId == ActiveLifeId)
        {
            OldestReplicatedSequence = FMath::Min(
                OldestReplicatedSequence, Point.PointSequence);
        }
    }

    if (OldestReplicatedSequence == MAX_uint64)
    {
        return false;
    }

    int32 FirstIndex = 0;

    while (FirstIndex + 1 < PointCount
        && CachedPoints[FirstIndex + 1].PathDistance <= TailPathDistance)
    {
        ++FirstIndex;
    }

    uint64 ExpectedSequence = CachedPoints[FirstIndex].PointSequence;

    for (int32 PointIndex = FirstIndex; PointIndex < PointCount; ++PointIndex)
    {
        const FSlitherPathPoint& Point = CachedPoints[PointIndex];

        if (ExpectedSequence > Snapshot.RequiredPathPointSequence)
        {
            break;
        }

        if (Point.PointSequence != ExpectedSequence)
        {
            return OldestReplicatedSequence > ExpectedSequence;
        }

        ++ExpectedSequence;
    }

    return false;
}

bool USlitherBodyComponent::TrySamplePath(
    const TArray<FSlitherPathPoint>& Points,
    const double PathDistance,
    const FVector& HeadLocation,
    const FVector& HeadDirection,
    const double CurrentHeadPathDistance,
    FVector& OutLocation,
    FVector& OutDirection)
{
    const int32 PointCount = Points.Num();
    int32 LowerIndex = INDEX_NONE;

    for (int32 PointIndex = 0; PointIndex < PointCount; ++PointIndex)
    {
        if (Points[PointIndex].PathDistance > PathDistance)
        {
            break;
        }

        LowerIndex = PointIndex;
    }

    if (LowerIndex == INDEX_NONE || PathDistance > CurrentHeadPathDistance)
    {
        return false;
    }

    const FSlitherPathPoint& LowerPoint = Points[LowerIndex];
    FVector UpperLocation = HeadLocation;
    double UpperDistance = CurrentHeadPathDistance;
    const int32 UpperIndex = LowerIndex + 1;

    if (UpperIndex < PointCount
        && Points[UpperIndex].PathDistance <= CurrentHeadPathDistance)
    {
        UpperLocation = Points[UpperIndex].Location;
        UpperDistance = Points[UpperIndex].PathDistance;
    }

    if (PathDistance > UpperDistance)
    {
        return false;
    }

    const double Interval = UpperDistance - LowerPoint.PathDistance;

    if (Interval > 0.0)
    {
        const double Alpha =
            (PathDistance - LowerPoint.PathDistance) / Interval;
        OutLocation = FMath::Lerp(
            LowerPoint.Location, UpperLocation, Alpha);
    }
    else
    {
        OutLocation = LowerPoint.Location;
    }

    const FVector DirectionDelta = UpperLocation - LowerPoint.Location;
    const double DirectionSizeSquared = DirectionDelta.SizeSquared();

    if (DirectionSizeSquared > UE_SMALL_NUMBER)
    {
        OutDirection = DirectionDelta.GetSafeNormal();
    }
    else
    {
        OutDirection = HeadDirection;
    }

    return true;
}

void USlitherBodyComponent::UpdateServerSegments()
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const FVector HeadLocation = WormPawn->GetActorLocation();
    const FVector HeadDirection = WormPawn->GetActorForwardVector();
    const TArray<FSlitherPathPoint>& Points = ReplicatedPath.GetPoints();
    const int32 SegmentCount = CalculateSegmentCount(BodyLength);

    ServerSegmentLocations.SetNum(SegmentCount, EAllowShrinking::No);

    for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        const double SegmentOffset = CalculateSegmentOffset(SegmentIndex, BodyLength);
        const double SegmentDistance = HeadPathDistance - SegmentOffset;
        FVector SegmentLocation;
        FVector SegmentDirection;
        const bool SegmentFound = TrySamplePath(
            Points, SegmentDistance, HeadLocation, HeadDirection,
            HeadPathDistance, SegmentLocation, SegmentDirection);
        check(SegmentFound);

        if (!SegmentFound)
        {
            return;
        }

        ServerSegmentLocations[SegmentIndex] = SegmentLocation;
    }
}

bool USlitherBodyComponent::TryBuildVisualBody(
    const FVector& HeadLocation,
    const FVector& HeadDirection,
    const double RenderPathDistance,
    const double RenderBodyLength,
    const uint64 RequiredPathPointSequence,
    TArray<FTransform>& OutTransforms)
{
    WaitingForPointSequence = RequiredPathPointSequence;
    
    if (ActiveLifeId == 0 || RequiredPathPointSequence == 0
        || RenderBodyLength < FSlitherSettingsGrowth::InitialBodyLength
        || RenderBodyLength > FSlitherSettingsGrowth::MaxBodyLength)
    {
        return false;
    }

    const bool HasRequiredHistory = HasVisualPathRange(
        RenderPathDistance, RenderBodyLength, RequiredPathPointSequence);

    if (!HasRequiredHistory)
    {
        return false;
    }

    const int32 SegmentCount = CalculateSegmentCount(RenderBodyLength);
    constexpr double ScaleFactor =
        FSlitherSettingsBody::BodySegmentRadius
        / FSlitherSettingsBody::UnitSphereRadius;
    const FVector SegmentScale(ScaleFactor);
    OutTransforms.SetNum(SegmentCount, EAllowShrinking::No);

    for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        const double SegmentOffset = CalculateSegmentOffset(SegmentIndex, RenderBodyLength);
        const double SegmentDistance = RenderPathDistance - SegmentOffset;
        FVector SegmentLocation;
        FVector SegmentDirection;
        const bool SegmentFound = TrySamplePath(
            CachedPoints, SegmentDistance, HeadLocation, HeadDirection,
            RenderPathDistance, SegmentLocation, SegmentDirection);

        if (!SegmentFound)
        {
            return false;
        }

        const FRotator SegmentRotation = SegmentDirection.Rotation();
        const FTransform SegmentTransform(
            SegmentRotation, SegmentLocation, SegmentScale);
        OutTransforms[SegmentIndex] = SegmentTransform;
    }

    WaitingForPointSequence = 0;
    return true;
}

void USlitherBodyComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USlitherBodyComponent, ReplicatedPath);
}
