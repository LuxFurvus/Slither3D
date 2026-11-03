
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "SlitherBodyTypes.generated.h"

class USlitherBodyComponent;


USTRUCT()
struct FSlitherPathPoint : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:

    UPROPERTY()
    uint32 LifeId = 0;

    UPROPERTY()
    uint64 PointSequence = 0;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    double PathDistance = 0.0;
};


USTRUCT()
struct FSlitherReplicatedPath : public FFastArraySerializer
{
    GENERATED_BODY()

private:

    TWeakObjectPtr<USlitherBodyComponent> OwnerComponent;

    UPROPERTY()
    TArray<FSlitherPathPoint> Points;

private:

    void CacheChangedPoints(
        const TArrayView<int32>& PointIndices) const;

public:

    void SetOwnerComponent(
        USlitherBodyComponent* InOwnerComponent);

    const TArray<FSlitherPathPoint>& GetPoints() const;

    void Reset();

    void AddPoint(
        const FSlitherPathPoint& Point);

    void TrimBefore(
        const double MinimumDistance,
        const int32 MaximumPointCount);

    bool NetDeltaSerialize(
        FNetDeltaSerializeInfo& DeltaParameters);

    void PreReplicatedRemove(
        const TArrayView<int32>& RemovedIndices,
        const int32 FinalSize) const;

    void PostReplicatedAdd(
        const TArrayView<int32>& AddedIndices,
        const int32 FinalSize) const;

    void PostReplicatedChange(
        const TArrayView<int32>& ChangedIndices,
        const int32 FinalSize) const;

    void PostReplicatedReceive(
        const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters) const;
};


template<>
struct TStructOpsTypeTraits<FSlitherReplicatedPath>
    : public TStructOpsTypeTraitsBase2<FSlitherReplicatedPath>
{
public:

    static constexpr bool WithNetDeltaSerializer = true;
};
