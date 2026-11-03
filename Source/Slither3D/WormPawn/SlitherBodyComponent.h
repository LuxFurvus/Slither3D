
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherBodyTypes.h"
#include "SlitherWormMovementTypes.h"

#include "SlitherBodyComponent.generated.h"

class ASlitherWormPawn;


UCLASS()
class USlitherBodyComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    uint32 ActiveLifeId = 0;
    uint64 NextPointSequence = 1;
    uint64 WaitingForPointSequence = 0;

    double HeadPathDistance = 0.0;
    double NextSampleDistance = 0.0;
    double BodyLength = 0.0;

    TArray<FVector> ServerSegmentLocations;
    TArray<FSlitherPathPoint> CachedPoints;

    UPROPERTY(Replicated)
    FSlitherReplicatedPath ReplicatedPath;

private:

    USlitherBodyComponent();

    ASlitherWormPawn* GetWormPawn() const;

    void AppendPathPoint(
        const FVector& Location,
        const double PathDistance);

    void ImportReplicatedPath();

    void UpdateServerSegments();

    static int32 CalculateSegmentCount(const double Length);

    static double CalculateSegmentOffset(
        const int32 SegmentIndex,
        const double Length);

    bool HasVisualPathRange(
        const double RenderPathDistance,
        const double RenderBodyLength,
        const uint64 RequiredPathPointSequence) const;

    static bool ComparePathPoints(
        const FSlitherPathPoint& FirstPoint,
        const FSlitherPathPoint& SecondPoint);

    static bool TrySamplePath(
        const TArray<FSlitherPathPoint>& Points,
        const double PathDistance,
        const FVector& HeadLocation,
        const FVector& HeadDirection,
        const double CurrentHeadPathDistance,
        FVector& OutLocation,
        FVector& OutDirection);

protected:

    void OnRegister() override;

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    void InitializeLife(
        const uint32 LifeId,
        const FVector& HeadLocation,
        const FVector& HeadDirection);

    void ResetForLife(
        const uint32 LifeId);

    void RecordHeadMovement(
        const FVector& PreviousHeadLocation,
        const FVector& CurrentHeadLocation);

    void RefreshServerSegments();

    void ApplyGrowth(const int32 Mass, const double MovedDistance);

    double GetHeadPathDistance() const;

    double GetBodyLength() const;

    uint64 GetLastPathPointSequence() const;

    const TArray<FVector>& GetServerSegmentLocations() const;

    int32 GetServerPathPointCount() const;

    int32 GetCachedPathPointCount() const;

    uint64 GetWaitingForPointSequence() const;

    void CachePathPoint(
        const FSlitherPathPoint& Point);

    void FinishPathUpdate();

    void TrimCachedPath(
        const double MinimumDistance);

    bool IsSnapshotObsolete(
        const FSlitherWormSnapshot& Snapshot) const;

    bool TryBuildVisualBody(
        const FVector& HeadLocation,
        const FVector& HeadDirection,
        const double RenderPathDistance,
        const double RenderBodyLength,
        const uint64 RequiredPathPointSequence,
        TArray<FTransform>& OutTransforms);

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
