
#include "SlitherArena.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AssertionMacros.h"
#include "UObject/ConstructorHelpers.h"

#include "Settings/SlitherGameSettingsTypes.h"


ASlitherArena::ASlitherArena()
{
    PrimaryActorTick.bCanEverTick = false;

    InitComponents();

    ImplementStaticMeshes();

    UpdateGeometry();
}

void ASlitherArena::InitComponents()
{
    ArenaRoot
        = CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot"));
    SetRootComponent(ArenaRoot);
    ArenaRoot->SetMobility(EComponentMobility::Static);

    Floor
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
    Floor->SetupAttachment(ArenaRoot);
    Floor->SetMobility(EComponentMobility::Static);

    NegativeXWall
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NegativeXWall"));
    NegativeXWall->SetupAttachment(ArenaRoot);
    NegativeXWall->SetMobility(EComponentMobility::Static);

    PositiveXWall
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PositiveXWall"));
    PositiveXWall->SetupAttachment(ArenaRoot);
    PositiveXWall->SetMobility(EComponentMobility::Static);

    NegativeYWall
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NegativeYWall"));
    NegativeYWall->SetupAttachment(ArenaRoot);
    NegativeYWall->SetMobility(EComponentMobility::Static);

    PositiveYWall
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PositiveYWall"));
    PositiveYWall->SetupAttachment(ArenaRoot);
    PositiveYWall->SetMobility(EComponentMobility::Static);
}

void ASlitherArena::ImplementStaticMeshes() const
{
    const ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    const bool CubeMeshFound = CubeMeshFinder.Succeeded();
    check(CubeMeshFound);

    UStaticMesh* CubeMesh = CubeMeshFinder.Object;

    ConfigureGeometryComponent(Floor, CubeMesh);
    ConfigureGeometryComponent(NegativeXWall, CubeMesh);
    ConfigureGeometryComponent(PositiveXWall, CubeMesh);
    ConfigureGeometryComponent(NegativeYWall, CubeMesh);
    ConfigureGeometryComponent(PositiveYWall, CubeMesh);
}

void ASlitherArena::ConfigureGeometryComponent(
    UStaticMeshComponent* GeometryComponent,
    UStaticMesh* CubeMesh)
{
    const FName CollisionProfileName(TEXT("BlockAll"));

    GeometryComponent->SetStaticMesh(CubeMesh);
    GeometryComponent->SetCollisionProfileName(CollisionProfileName, false);
    GeometryComponent->SetCollisionObjectType(ECC_WorldStatic);
    GeometryComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GeometryComponent->SetSimulatePhysics(false);
    GeometryComponent->SetGenerateOverlapEvents(false);
}

void ASlitherArena::UpdateGeometry() const
{
    constexpr double Width = FSlitherSettingsArena::ArenaWidth;
    constexpr double Height = FSlitherSettingsArena::ArenaHeight;
    constexpr double FloorThickness = FSlitherSettingsArena::FloorThickness;
    constexpr double WallThickness = FSlitherSettingsArena::WallThickness;
    constexpr double WallHeight = FSlitherSettingsArena::WallHeight;

    constexpr double HalfWidth = Width * 0.5;
    constexpr double HalfHeight = Height * 0.5;
    constexpr double HalfFloorThickness = FloorThickness * 0.5;
    constexpr double HalfWallThickness = WallThickness * 0.5;
    constexpr double HalfWallHeight = WallHeight * 0.5;

    constexpr double OuterWidth = Width + WallThickness * 2.0;
    constexpr double OuterHeight = Height + WallThickness * 2.0;
    constexpr double XWallOffset = HalfWidth + HalfWallThickness;
    constexpr double YWallOffset = HalfHeight + HalfWallThickness;

    const FVector FloorDimensions(OuterWidth, OuterHeight, FloorThickness);
    const FVector FloorScale = FloorDimensions / UnitCubeSize;
    const FVector FloorLocation(0.0, 0.0, -HalfFloorThickness);

    Floor->SetRelativeScale3D(FloorScale);
    Floor->SetRelativeLocation(FloorLocation);

    const FVector XWallDimensions(WallThickness, OuterHeight, WallHeight);
    const FVector XWallScale = XWallDimensions / UnitCubeSize;
    const FVector NegativeXWallLocation(-XWallOffset, 0.0, HalfWallHeight);
    const FVector PositiveXWallLocation(XWallOffset, 0.0, HalfWallHeight);

    NegativeXWall->SetRelativeScale3D(XWallScale);
    NegativeXWall->SetRelativeLocation(NegativeXWallLocation);
    PositiveXWall->SetRelativeScale3D(XWallScale);
    PositiveXWall->SetRelativeLocation(PositiveXWallLocation);

    const FVector YWallDimensions(Width, WallThickness, WallHeight);
    const FVector YWallScale = YWallDimensions / UnitCubeSize;
    const FVector NegativeYWallLocation(0.0, -YWallOffset, HalfWallHeight);
    const FVector PositiveYWallLocation(0.0, YWallOffset, HalfWallHeight);

    NegativeYWall->SetRelativeScale3D(YWallScale);
    NegativeYWall->SetRelativeLocation(NegativeYWallLocation);
    PositiveYWall->SetRelativeScale3D(YWallScale);
    PositiveYWall->SetRelativeLocation(PositiveYWallLocation);
}

void ASlitherArena::OnConstruction(
    const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    UpdateGeometry();
}

double ASlitherArena::GetWormHeadPlaneZ() const
{
    const FVector ArenaLocation
        = GetActorLocation();

    const double WormHeadPlaneZ
        = ArenaLocation.Z
        + FSlitherSettingsBody::HeadRadius;

    return WormHeadPlaneZ;
}

FPlane ASlitherArena::GetWormMovementPlane() const
{
    FVector PlaneOrigin = GetActorLocation();

    const double WormHeadPlaneZ = GetWormHeadPlaneZ();

    PlaneOrigin.Z = WormHeadPlaneZ;

    const FPlane GameplayPlane(
        PlaneOrigin, FVector::UpVector);

    return GameplayPlane;
}

FBox2D ASlitherArena::GetPlayableBounds() const
{
    constexpr double HalfWidth
        = FSlitherSettingsArena::ArenaWidth * 0.5;
    constexpr double HalfHeight
        = FSlitherSettingsArena::ArenaHeight * 0.5;

    const FVector ArenaLocation = GetActorLocation();

    const FVector2D Minimum(
        ArenaLocation.X - HalfWidth,
        ArenaLocation.Y - HalfHeight);

    const FVector2D Maximum(
        ArenaLocation.X + HalfWidth,
        ArenaLocation.Y + HalfHeight);

    const FBox2D PlayableBounds(Minimum, Maximum);

    return PlayableBounds;
}

bool ASlitherArena::CanPlaceCircle(
    const FVector2D& Center,
    const double Radius) const
{
    check(Radius > 0.0);

    const FBox2D Bounds = GetPlayableBounds();

    const double MinimumX = Bounds.Min.X + Radius;
    const double MaximumX = Bounds.Max.X - Radius;
    const double MinimumY = Bounds.Min.Y + Radius;
    const double MaximumY = Bounds.Max.Y - Radius;

    const bool bCanPlaceCircle
        = Center.X >= MinimumX
        && Center.X <= MaximumX
        && Center.Y >= MinimumY
        && Center.Y <= MaximumY;

    return bCanPlaceCircle;
}