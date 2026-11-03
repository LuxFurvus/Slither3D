
#include "SlitherWormPawn.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include "SlitherBodyComponent.h"
#include "SlitherWormMovementComponent.h"
#include "SlitherWormReplicationComponent.h"
#include "SlitherWormVisualComponent.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


ASlitherWormPawn::ASlitherWormPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI = EAutoPossessAI::Disabled;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    bReplicates = true;
    AActor::SetReplicateMovement(false);
    SetNetUpdateFrequency(FSlitherSettingsNetwork::WormStatePublishRate);
    SetMinNetUpdateFrequency(FSlitherSettingsNetwork::WormStatePublishRate);
    bAlwaysRelevant = true;

    HeadCollision
        = CreateDefaultSubobject<USphereComponent>(TEXT("HeadCollision"));
    SetRootComponent(HeadCollision);

    constexpr float HeadRadius = FSlitherSettingsBody::HeadRadius;

    HeadCollision->SetSphereRadius(HeadRadius, false);
    HeadCollision->SetSimulatePhysics(false);
    HeadCollision->SetEnableGravity(false);
    HeadCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadCollision->SetCollisionObjectType(ECC_Pawn);
    HeadCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    HeadCollision->SetGenerateOverlapEvents(false);

    WormRoot
        = CreateDefaultSubobject<USceneComponent>(TEXT("WormRoot"));
    WormRoot->SetupAttachment(HeadCollision);
    WormRoot->SetAbsolute(true, true, false);

    HeadMesh
        = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(WormRoot);
    HeadMesh->SetSimulatePhysics(false);
    HeadMesh->SetEnableGravity(false);
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadMesh->SetGenerateOverlapEvents(false);
    HeadMesh->SetVisibility(false);

    BodyMesh
        = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(WormRoot);
    BodyMesh->SetSimulatePhysics(false);
    BodyMesh->SetEnableGravity(false);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetGenerateOverlapEvents(false);
    BodyMesh->SetCanEverAffectNavigation(false);
    BodyMesh->SetIsReplicated(false);
    BodyMesh->SetVisibility(false);

    constexpr double HeadScaleFactor
        = HeadRadius / FSlitherSettingsBody::UnitSphereRadius;

    const FVector HeadScale(HeadScaleFactor);

    HeadMesh->SetRelativeScale3D(HeadScale);

    const ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    const bool SphereMeshFound = SphereMeshFinder.Succeeded();
    check(SphereMeshFound);

    UStaticMesh* SphereMesh = SphereMeshFinder.Object;

    HeadMesh->SetStaticMesh(SphereMesh);
    BodyMesh->SetStaticMesh(SphereMesh);

    WormBodyComponent
        = CreateDefaultSubobject<USlitherBodyComponent>(
            TEXT("WormBodyComponent"));

    WormMovementComponent
        = CreateDefaultSubobject<USlitherWormMovementComponent>(
            TEXT("WormMovementComponent"));

    WormReplicationComponent
        = CreateDefaultSubobject<USlitherWormReplicationComponent>(
            TEXT("WormReplicationComponent"));

    WormVisualComponent
        = CreateDefaultSubobject<USlitherWormVisualComponent>(
            TEXT("WormVisualComponent"));
}

FVector ASlitherWormPawn::GetVisualHeadLocation() const
{
    check(WormVisualComponent);

    const FVector VisualHeadLocation
        = WormVisualComponent->GetVisualHeadLocation();

    return VisualHeadLocation;
}

uint32 ASlitherWormPawn::GetLifeId() const
{
    return LifeId;
}

bool ASlitherWormPawn::HasActiveLife() const
{
    const ASlitherPlayerState* SlitherPlayerState
        = GetPlayerState<ASlitherPlayerState>();

    if (!SlitherPlayerState || LifeId == 0)
    {
        return false;
    }

    const bool PawnBeingDestroyed
        = IsActorBeingDestroyed();

    const bool PlayerBeingDestroyed
        = SlitherPlayerState->IsActorBeingDestroyed();

    const uint32 PlayerLifeId
        = SlitherPlayerState->GetLifeId();

    const bool Alive
        = SlitherPlayerState->IsAlive();

    const bool bHasActiveLife
        = !PawnBeingDestroyed && !PlayerBeingDestroyed
        && Alive && PlayerLifeId == LifeId;

    return bHasActiveLife;
}

USlitherBodyComponent* ASlitherWormPawn::GetBodyComponent() const
{
    check(WormBodyComponent);

    USlitherBodyComponent* BodyComponent
        = WormBodyComponent.Get();

    return BodyComponent;
}

const FSlitherWormInputState& ASlitherWormPawn::GetInputState() const
{
    check(WormMovementComponent);

    const FSlitherWormInputState& InputState
        = WormMovementComponent->GetInputState();

    return InputState;
}

bool ASlitherWormPawn::InitializeLife(const uint32 NewLifeId)
{
    check(WormBodyComponent);
    check(WormVisualComponent);
    check(WormMovementComponent);

    const bool HasServerAuthority = HasAuthority();
    const bool BeingDestroyed = IsActorBeingDestroyed();

    if (!HasServerAuthority || BeingDestroyed || NewLifeId == 0 || LifeId > 0)
    {
        return false;
    }

    const FVector HeadLocation = GetActorLocation();
    const FVector HeadDirection = GetActorForwardVector();

    LifeId = NewLifeId;
    ResetInputState();
    WormBodyComponent->InitializeLife(LifeId, HeadLocation, HeadDirection);
    WormVisualComponent->ResetForLife(LifeId);

    const TArray<FVector>& Segments
        = WormBodyComponent->GetServerSegmentLocations();
    const int32 SegmentCount
        = Segments.Num();

    const bool IsCorrectSegmentCount
        = SegmentCount == FSlitherSettingsGrowth::InitialElementCount - 1;

    return IsCorrectSegmentCount;
}

void ASlitherWormPawn::ResetInputState() const
{
    WormMovementComponent->ResetInputState();
}

bool ASlitherWormPawn::SubmitInput(
    const uint32 InputLifeId,
    const uint32 InputSequence,
    const FVector& AimDirection,
    const bool WantsBoost,
    const double InputTime) const
{
    const bool InputAccepted = WormMovementComponent->SubmitInput(
        InputLifeId, InputSequence, AimDirection, WantsBoost, InputTime);

    return InputAccepted;
}

void ASlitherWormPawn::SimulateMovement(
    const ASlitherArena* Arena,
    const double StepSeconds,
    const double ServerTime,
    FSlitherWormMovementResult& OutResult) const
{
    check(WormBodyComponent);
    check(WormMovementComponent);

    const FSlitherWormMovementResult InitialResult;
    OutResult = InitialResult;

    if (const bool HasServerAuthority
            = HasAuthority();
        !HasServerAuthority)
    {
        return;
    }

    const FVector PreviousHeadLocation = GetActorLocation();
    WormMovementComponent->SimulateMovement(
        Arena, StepSeconds, ServerTime, OutResult);

    const FVector CurrentHeadLocation = GetActorLocation();
    OutResult.CurrentHeadLocation = CurrentHeadLocation;
    WormBodyComponent->RecordHeadMovement(
        PreviousHeadLocation, CurrentHeadLocation);
}

void ASlitherWormPawn::PublishSnapshot(
    const uint32 SimulationFrame,
    const double ServerTime) const
{
    check(WormMovementComponent);
    check(WormReplicationComponent);

    const float CurrentSpeed
        = WormMovementComponent->GetCurrentSpeed();

    WormReplicationComponent->PublishSnapshot(
        SimulationFrame, ServerTime, CurrentSpeed);
}

void ASlitherWormPawn::UpdateTailMaterial()
{
    check(BodyMesh);

    if (const ENetMode NetMode = GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return;
    }

    UMaterialInterface* Material
        = WormTailMaterial.Get();

    if (!Material)
    {
        return;
    }

    if (!WormTailDynamicMaterial)
    {
        WormTailDynamicMaterial
            = BodyMesh->CreateDynamicMaterialInstance(
                0, Material);
    }

    if (!WormTailDynamicMaterial)
    {
        return;
    }

    WormTailDynamicMaterial->SetVectorParameterValue(
        WormColorParameterName, WormColor);
}

void ASlitherWormPawn::OnRep_LifeId() const
{
    check(WormBodyComponent);
    check(WormVisualComponent);
    check(WormReplicationComponent);

    WormBodyComponent->ResetForLife(LifeId);
    WormVisualComponent->ResetForLife(LifeId);
    WormReplicationComponent->ProcessLatestSnapshot();
}

void ASlitherWormPawn::OnRep_WormColor()
{
    UpdateTailMaterial();
}

void ASlitherWormPawn::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    check(WormVisualComponent);

    WormVisualComponent->InitializeVisualComponents(
        WormRoot, HeadMesh, BodyMesh, WormBodyComponent);
}

void ASlitherWormPawn::BeginPlay()
{
    Super::BeginPlay();

    check(WormBodyComponent);
    check(WormVisualComponent);
    check(WormReplicationComponent);

    if (const bool HasServerAuthority
        = HasAuthority())
    {
        WormColor = FLinearColor::MakeRandomColor();
    }

    UpdateTailMaterial();
    WormBodyComponent->ResetForLife(LifeId);
    WormVisualComponent->InitializeVisualState();
    WormReplicationComponent->ProcessLatestSnapshot();
}

void ASlitherWormPawn::UpdateVisualState() const
{
    check(WormVisualComponent);

    WormVisualComponent->UpdateVisualState();
}

bool ASlitherWormPawn::TryBufferVisualSnapshot(
    const FSlitherWormSnapshot& Snapshot) const
{
    check(WormVisualComponent);

    const bool SnapshotAccepted
        = WormVisualComponent->TryBufferSnapshot(Snapshot);

    return SnapshotAccepted;
}

void ASlitherWormPawn::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASlitherWormPawn, LifeId);
    DOREPLIFETIME(ASlitherWormPawn, WormColor);
}
