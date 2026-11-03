
#include "SlitherBodyTypes.h"

#include "SlitherBodyComponent.h"


void FSlitherReplicatedPath::SetOwnerComponent(
    USlitherBodyComponent* InOwnerComponent)
{
    OwnerComponent = InOwnerComponent;
}

const TArray<FSlitherPathPoint>& FSlitherReplicatedPath::GetPoints() const
{
    return Points;
}

void FSlitherReplicatedPath::Reset()
{
    Points.Reset();
    MarkArrayDirty();
}

void FSlitherReplicatedPath::AddPoint(
    const FSlitherPathPoint& Point)
{
    FSlitherPathPoint& AddedPoint = Points.Add_GetRef(Point);
    MarkItemDirty(AddedPoint);
}

void FSlitherReplicatedPath::TrimBefore(
    const double MinimumDistance,
    const int32 MaximumPointCount)
{
    const int32 PointCount = Points.Num();
    int32 RemoveCount = 0;

    while (RemoveCount + 1 < PointCount
        && Points[RemoveCount + 1].PathDistance <= MinimumDistance)
    {
        ++RemoveCount;
    }

    const int32 ExcessCount = PointCount - MaximumPointCount;
    RemoveCount = FMath::Max(RemoveCount, ExcessCount);

    if (RemoveCount > 0)
    {
        Points.RemoveAt(0, RemoveCount, EAllowShrinking::No);
        MarkArrayDirty();
    }
}

bool FSlitherReplicatedPath::NetDeltaSerialize(
    FNetDeltaSerializeInfo& DeltaParameters)
{
    const bool SerializationSucceeded =
        FFastArraySerializer::FastArrayDeltaSerialize<
            FSlitherPathPoint,
            FSlitherReplicatedPath>(
                Points, DeltaParameters, *this);

    return SerializationSucceeded;
}

void FSlitherReplicatedPath::CacheChangedPoints(
    const TArrayView<int32>& PointIndices) const
{
    USlitherBodyComponent* BodyComponent = OwnerComponent.Get();

    if (!BodyComponent)
    {
        return;
    }

    for (const int32 PointIndex : PointIndices)
    {
        const FSlitherPathPoint& Point = Points[PointIndex];
        BodyComponent->CachePathPoint(Point);
    }
}

void FSlitherReplicatedPath::PreReplicatedRemove(
    const TArrayView<int32>& RemovedIndices,
    const int32 FinalSize) const
{
    CacheChangedPoints(RemovedIndices);
}

void FSlitherReplicatedPath::PostReplicatedAdd(
    const TArrayView<int32>& AddedIndices,
    const int32 FinalSize) const
{
    CacheChangedPoints(AddedIndices);
}

void FSlitherReplicatedPath::PostReplicatedChange(
    const TArrayView<int32>& ChangedIndices,
    const int32 FinalSize) const
{
    CacheChangedPoints(ChangedIndices);
}

void FSlitherReplicatedPath::PostReplicatedReceive(
    const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters) const
{
    if (USlitherBodyComponent* BodyComponent = OwnerComponent.Get())
    {
        BodyComponent->FinishPathUpdate();
    }
}
