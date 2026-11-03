
#include "SlitherWormSpawnComponent.h"

#include "Engine/World.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/Controller.h"

#include "Slither3D/SlitherArena.h"
#include "Slither3D/WormPawn/SlitherBodyComponent.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


bool USlitherWormSpawnComponent::BuildObstacleGrid()
{
    Obstacles.Reset();
    ObstacleCells.Reset();

    for (const TWeakObjectPtr<ASlitherWormPawn>& PawnReference : WormPawns)
    {
        const ASlitherWormPawn* WormPawn = PawnReference.Get();

        if (!WormPawn)
        {
            continue;
        }

        const bool BeingDestroyed = WormPawn->IsActorBeingDestroyed();
        const bool Active = WormPawn->HasActiveLife();

        if (BeingDestroyed || !Active)
        {
            continue;
        }

        const USlitherBodyComponent* Body = WormPawn->GetBodyComponent();

        checkf(Body,
            TEXT("Cannot spawn: active worm has no body."));

        const TArray<FVector>& Segments = Body->GetServerSegmentLocations();
        const int32 SegmentCount = Segments.Num();
        const FVector HeadLocation = WormPawn->GetActorLocation();

        checkf(SegmentCount >= FSlitherSettingsGrowth::InitialElementCount - 1,
            TEXT("Cannot spawn: active worm geometry is incomplete."));

        AddObstacle(HeadLocation, FSlitherSettingsBody::HeadRadius);

        for (const FVector& Segment : Segments)
        {

            AddObstacle(Segment, FSlitherSettingsBody::BodySegmentRadius);
        }
    }

    return true;
}

void USlitherWormSpawnComponent::AddObstacle(
    const FVector& Location,
    const double Radius)
{
    FSlitherSpawnObstacle Obstacle;
    Obstacle.Center = FVector2D(Location.X, Location.Y);
    Obstacle.Radius = Radius;

    const int32 ObstacleIndex
        = Obstacles.Emplace(Obstacle);

    FIntPoint Minimum;
    FIntPoint Maximum;
    GetCellBounds(Obstacle.Center, Radius, Minimum, Maximum);

    for (int32 Y = Minimum.Y; Y <= Maximum.Y; ++Y)
    {
        for (int32 X = Minimum.X; X <= Maximum.X; ++X)
        {
            const FIntPoint Cell(X, Y);
            TArray<int32>& Indices
                = ObstacleCells.FindOrAdd(Cell);
            Indices.Emplace(ObstacleIndex);
        }
    }
}

void USlitherWormSpawnComponent::GetCellBounds(
    const FVector2D& Center,
    const double Radius,
    FIntPoint& OutMinimum,
    FIntPoint& OutMaximum)
{
    constexpr double CellSize = FSlitherSettingsCollision::CellSize;
    OutMinimum.X = FMath::FloorToInt((Center.X - Radius) / CellSize);
    OutMinimum.Y = FMath::FloorToInt((Center.Y - Radius) / CellSize);
    OutMaximum.X = FMath::FloorToInt((Center.X + Radius) / CellSize);
    OutMaximum.Y = FMath::FloorToInt((Center.Y + Radius) / CellSize);
}

bool USlitherWormSpawnComponent::CanPlaceElement(
    const ASlitherArena* Arena,
    const FVector2D& Center,
    const double Radius)
{
    const double RequiredRadius = Radius + FSlitherSettingsRespawn::SpawnClearance;
    const bool FitsArena = Arena->CanPlaceCircle(Center, RequiredRadius);

    if (!FitsArena)
    {
        return false;
    }

    CandidateIndices.Reset();
    FIntPoint Minimum;
    FIntPoint Maximum;
    GetCellBounds(Center, RequiredRadius, Minimum, Maximum);

    for (int32 Y = Minimum.Y; Y <= Maximum.Y; ++Y)
    {
        for (int32 X = Minimum.X; X <= Maximum.X; ++X)
        {
            const FIntPoint Cell(X, Y);
            const TArray<int32>* Indices = ObstacleCells.Find(Cell);

            if (!Indices)
            {
                continue;
            }

            for (const int32 Index : *Indices)
            {
                CandidateIndices.Emplace(Index);
            }
        }
    }

    for (const int32 Index : CandidateIndices)
    {
        const FSlitherSpawnObstacle& Obstacle = Obstacles[Index];
        const FVector2D Delta = Center - Obstacle.Center;
        const double DistanceSquared = Delta.SizeSquared();
        const double RequiredDistance = RequiredRadius + Obstacle.Radius;
        const double RequiredDistanceSquared = RequiredDistance * RequiredDistance;

        if (DistanceSquared < RequiredDistanceSquared)
        {
            return false;
        }
    }

    return true;
}

TOptional<FTransform> USlitherWormSpawnComponent::TryBuildSpawnTransform(
    const ASlitherArena* Arena,
    const FVector2D& Center,
    const FVector2D& ArenaCenter)
{
    const FVector2D ToCenter
        = ArenaCenter - Center;
    double Yaw = 0.0;

    if (const bool HasDirection
        = ToCenter.X != 0.0
        || ToCenter.Y != 0.0)
    {
        const double YawRadians
            = FMath::Atan2(ToCenter.Y, ToCenter.X);

        Yaw = FMath::RadiansToDegrees(YawRadians);
    }

    const FRotator Rotation(0.0, Yaw, 0.0);
    const FVector Direction = Rotation.Vector();

    for (int32 ElementIndex = 0;
         ElementIndex < FSlitherSettingsGrowth::InitialElementCount;
         ++ElementIndex)
    {
        const double Offset
            = ElementIndex
            * FSlitherSettingsBody::SegmentSpacing;

        const FVector2D ElementCenter(
            Center.X - Direction.X * Offset,
            Center.Y - Direction.Y * Offset);

        const double Radius = ElementIndex == 0
            ? FSlitherSettingsBody::HeadRadius
            : FSlitherSettingsBody::BodySegmentRadius;

        const bool Fits
            = CanPlaceElement(Arena, ElementCenter, Radius);

        if (!Fits)
        {
            return {};
        }
    }

    const double PlaneZ = Arena->GetWormHeadPlaneZ();
    const FVector Location(Center.X, Center.Y, PlaneZ);
    const FTransform Transform(Rotation, Location, FVector::OneVector);
    const TOptional<FTransform> Result(Transform);
    return Result;
}

void USlitherWormSpawnComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    WormPawns.Reset();
    Obstacles.Reset();
    ObstacleCells.Reset();
    CandidateIndices.Reset();
    Super::EndPlay(EndPlayReason);
}

USlitherWormSpawnComponent::USlitherWormSpawnComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

TOptional<FTransform> USlitherWormSpawnComponent::TryGetSpawnTransform(
    const ASlitherArena* Arena)
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor || !Arena)
    {
        return {};
    }

    const bool Authority = OwnerActor->HasAuthority();
    const bool ArenaBeingDestroyed = Arena->IsActorBeingDestroyed();

    if (!Authority || ArenaBeingDestroyed)
    {
        return {};
    }

    const bool HasObstacleGeometry = BuildObstacleGrid();

    if (!HasObstacleGeometry)
    {
        return {};
    }

    const FBox2D Bounds = Arena->GetPlayableBounds();

    const FVector2D ArenaCenter = Bounds.GetCenter();
    constexpr double Margin = FSlitherSettingsBody::HeadRadius
        + FSlitherSettingsRespawn::SpawnClearance;
    const FVector2D Minimum(Bounds.Min.X + Margin, Bounds.Min.Y + Margin);
    const FVector2D Maximum(Bounds.Max.X - Margin, Bounds.Max.Y - Margin);

    if (Minimum.X > Maximum.X || Minimum.Y > Maximum.Y)
    {
        return {};
    }

    for (int32 Attempt = 0;
         Attempt < FSlitherSettingsRespawn::RandomSpawnAttemptCount; ++Attempt)
    {
        const double X = FMath::FRandRange(Minimum.X, Maximum.X);
        const double Y = FMath::FRandRange(Minimum.Y, Maximum.Y);
        const FVector2D Center(X, Y);
        const TOptional<FTransform> Candidate =
            TryBuildSpawnTransform(Arena, Center, ArenaCenter);

        if (const bool Found
            = Candidate.IsSet())
        {
            return Candidate;
        }
    }

    constexpr double Spacing = FSlitherSettingsRespawn::SpawnGridSpacing;
    const int32 XIntervals = FMath::CeilToInt((Maximum.X - Minimum.X) / Spacing);
    const int32 YIntervals = FMath::CeilToInt((Maximum.Y - Minimum.Y) / Spacing);

    for (int32 YIndex = 0; YIndex <= YIntervals; ++YIndex)
    {
        const double Y = FMath::Min(Minimum.Y + YIndex * Spacing, Maximum.Y);

        for (int32 XIndex = 0; XIndex <= XIntervals; ++XIndex)
        {
            const double X
                = FMath::Min(Minimum.X + XIndex * Spacing, Maximum.X);
            const FVector2D Center(X, Y);

            const TOptional<FTransform> Candidate
                = TryBuildSpawnTransform(Arena, Center, ArenaCenter);

            if (const bool Found
                = Candidate.IsSet())
            {
                return Candidate;
            }
        }
    }

    return {};
}

ASlitherWormPawn* USlitherWormSpawnComponent::SpawnWorm(
    AController* NewPlayer,
    UClass* PawnClass,
    const FTransform& SpawnTransform) const
{
    const AActor* OwnerActor = GetOwner();
    UWorld* ThisWorld = GetWorld();

    if (!OwnerActor || !NewPlayer || !ThisWorld || !PawnClass)
    {
        return nullptr;
    }

    const bool Authority = OwnerActor->HasAuthority();
    UClass* WormPawnClass = ASlitherWormPawn::StaticClass();
    const bool PawnClassIsWorm = PawnClass->IsChildOf(WormPawnClass);

    if (!Authority || !PawnClassIsWorm)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = NewPlayer;
    SpawnParameters.SpawnCollisionHandlingOverride
        = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASlitherWormPawn* SpawnedPawn = ThisWorld->SpawnActor<ASlitherWormPawn>(
        PawnClass, SpawnTransform, SpawnParameters);
    return SpawnedPawn;
}

void USlitherWormSpawnComponent::RegisterWormPawn(
    ASlitherWormPawn* WormPawn)
{
    if (!WormPawn)
    {
        return;
    }

    const TWeakObjectPtr<ASlitherWormPawn> PawnReference(WormPawn);
    WormPawns.AddUnique(PawnReference);
}

void USlitherWormSpawnComponent::UnregisterWormPawn(
    const ASlitherWormPawn* WormPawn)
{
    const int32 PawnCount = WormPawns.Num();

    for (int32 Index = PawnCount - 1; Index >= 0; --Index)
    {
        const ASlitherWormPawn* RegisteredPawn = WormPawns[Index].Get();

        if (!RegisteredPawn || RegisteredPawn == WormPawn)
        {
            WormPawns.RemoveAt(Index, 1, EAllowShrinking::No);
        }
    }
}

const TArray<TWeakObjectPtr<ASlitherWormPawn>>&
USlitherWormSpawnComponent::GetWormPawns() const
{
    return WormPawns;
}
