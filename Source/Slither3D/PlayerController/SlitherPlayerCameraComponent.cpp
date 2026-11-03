
#include "SlitherPlayerCameraComponent.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"

#include "Slither3D/Slither3D.h"
#include "Slither3D/SlitherArena.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"


USlitherPlayerCameraComponent::USlitherPlayerCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void USlitherPlayerCameraComponent::ConfigureController(
    APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->bAutoManageActiveCameraTarget = false;
}

void USlitherPlayerCameraComponent::InitializeLocalCamera()
{
    AActor* OwnerActor = GetOwner();
    APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const ENetMode NetMode = PlayerController->GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return;
    }

    if (const bool ControllerIsLocal = PlayerController->IsLocalController();
        !ControllerIsLocal)
    {
        return;
    }

    if (const ACameraActor* CameraPointer = LocalCamera.Get())
    {
        if (const bool CameraIsBeingDestroyed
                = CameraPointer->IsActorBeingDestroyed();
            !CameraIsBeingDestroyed)
        {
            return;
        }

        LocalCamera = nullptr;
    }

    CameraHasTrackedPawn = false;

    ACameraActor* CreatedCamera = CreateLocalCamera();

    if (CreatedCamera == nullptr)
    {
        return;
    }

    LocalCamera = CreatedCamera;

    const FViewTargetTransitionParams TransitionParameters;
    PlayerController->SetViewTarget(CreatedCamera, TransitionParameters);
}

ACameraActor* USlitherPlayerCameraComponent::CreateLocalCamera()
{
    AActor* OwnerActor = GetOwner();
    APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return nullptr;
    }

    if (const ENetMode NetMode
            = PlayerController->GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return nullptr;
    }

    if (const bool ControllerIsLocal
            = PlayerController->IsLocalController();
        !ControllerIsLocal)
    {
        return nullptr;
    }

    check(LocalCamera == nullptr);
    check(CameraPitch >= -90.0f && CameraPitch < 0.0f);
    check(CameraDistance > 0.0f);
    check(CameraFieldOfView > 0.0f && CameraFieldOfView < 180.0f);
    check(CameraFollowSpeed > 0.0f);

    UWorld* ThisWorld = GetWorld();
    check(ThisWorld);

    FVector TargetLocation = FVector::ZeroVector;

    APawn* CurrentPawn = PlayerController->GetPawn();

    if (ASlitherWormPawn* WormPawn = Cast<ASlitherWormPawn>(CurrentPawn))
    {
        if (const bool PawnIsBeingDestroyed
                = WormPawn->IsActorBeingDestroyed();
            !PawnIsBeingDestroyed)
        {
            TargetLocation = WormPawn->GetVisualHeadLocation();
            CameraHasTrackedPawn = true;
        }
    }

    if (!CameraHasTrackedPawn)
    {
        ASlitherArena* FoundArena = nullptr;
        int32 ArenaCount = 0;

        for (TActorIterator<ASlitherArena> ArenaIterator(ThisWorld);
            ArenaIterator; ++ArenaIterator)
        {
            ASlitherArena* CandidateArena = *ArenaIterator;

            if (CandidateArena == nullptr)
            {
                continue;
            }

            if (const bool ArenaIsBeingDestroyed
                = CandidateArena->IsActorBeingDestroyed())
            {
                continue;
            }

            FoundArena = CandidateArena;
            ++ArenaCount;
        }

        if (ArenaCount == 1)
        {
            TargetLocation = FoundArena->GetActorLocation();
            TargetLocation.Z = FoundArena->GetWormHeadPlaneZ();
        }
        else
        {
            UE_LOG(SlitherLog,
                Warning, TEXT("Expected one arena for local camera, found %d; using world origin."),
                ArenaCount);
        }
    }

    const FRotator CameraRotation(CameraPitch, CameraYaw, 0.0f);
    const FVector ForwardDirection = CameraRotation.Vector();
    const FVector CameraOffset = ForwardDirection * CameraDistance;
    const FVector CameraLocation = TargetLocation - CameraOffset;
    const FTransform CameraTransform(
        CameraRotation, CameraLocation, FVector::OneVector);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = PlayerController;
    SpawnParameters.SpawnCollisionHandlingOverride
        = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UClass* CameraClass
        = ACameraActor::StaticClass();
    ACameraActor* CreatedCamera
        = ThisWorld->SpawnActor<ACameraActor>(
            CameraClass, CameraTransform, SpawnParameters);
    check(CreatedCamera);

    CreatedCamera->SetReplicates(false);
    CreatedCamera->SetReplicateMovement(false);

    UCameraComponent* CameraComponent
        = CreatedCamera->GetCameraComponent();
    checkf(CameraComponent,
        TEXT("Local camera has no camera component."));

    CameraComponent->SetProjectionMode(ECameraProjectionMode::Perspective);
    CameraComponent->SetFieldOfView(CameraFieldOfView);

    return CreatedCamera;
}

void USlitherPlayerCameraComponent::UpdateLocalCamera(
    const float DeltaSeconds)
{
    AActor* OwnerActor = GetOwner();
    APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const ENetMode NetMode = PlayerController->GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return;
    }

    if (const bool ControllerIsLocal
            = PlayerController->IsLocalController();
        !ControllerIsLocal)
    {
        return;
    }

    ACameraActor* CameraPointer = LocalCamera.Get();

    if (CameraPointer == nullptr)
    {
        return;
    }

    if (const bool CameraIsBeingDestroyed
        = CameraPointer->IsActorBeingDestroyed())
    {
        return;
    }

    if (const AActor* CurrentViewTarget = PlayerController->GetViewTarget();
        CurrentViewTarget != CameraPointer)
    {
        const FViewTargetTransitionParams TransitionParameters;
        PlayerController->SetViewTarget(CameraPointer, TransitionParameters);
    }

    APawn* CurrentPawn = PlayerController->GetPawn();
    const ASlitherWormPawn* WormPawn = Cast<ASlitherWormPawn>(CurrentPawn);

    if (WormPawn == nullptr)
    {
        return;
    }

    if (const bool PawnIsBeingDestroyed
        = WormPawn->IsActorBeingDestroyed())
    {
        return;
    }

    const FVector VisualHeadLocation = WormPawn->GetVisualHeadLocation();
    const FRotator CameraRotation(CameraPitch, CameraYaw, 0.0f);
    const FVector ForwardDirection = CameraRotation.Vector();
    const FVector CameraOffset = ForwardDirection * CameraDistance;
    const FVector DesiredCameraLocation = VisualHeadLocation - CameraOffset;

    FVector CameraLocation = DesiredCameraLocation;

    if (CameraHasTrackedPawn)
    {
        const FVector CurrentCameraLocation = CameraPointer->GetActorLocation();
        CameraLocation = FMath::VInterpTo(
            CurrentCameraLocation,
            DesiredCameraLocation,
            DeltaSeconds,
            CameraFollowSpeed);
    }

    CameraPointer->SetActorLocation(
        CameraLocation,
        false,
        nullptr,
        ETeleportType::None);
    CameraPointer->SetActorRotation(CameraRotation, ETeleportType::None);

    CameraHasTrackedPawn = true;
}

void USlitherPlayerCameraComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ACameraActor* CameraPointer = LocalCamera.Get();
    LocalCamera = nullptr;
    CameraHasTrackedPawn = false;

    if (CameraPointer)
    {
        if (const bool CameraIsBeingDestroyed
                = CameraPointer->IsActorBeingDestroyed();
            !CameraIsBeingDestroyed)
        {
            CameraPointer->Destroy();
        }
    }

    Super::EndPlay(EndPlayReason);
}

void USlitherPlayerCameraComponent::UpdateCamera(const float DeltaSeconds)
{
    AActor* OwnerActor = GetOwner();

    const APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const bool ControllerIsLocal
        = PlayerController->IsLocalController())
    {
        APawn* CurrentPawn = PlayerController->GetPawn();

        if (ASlitherWormPawn* WormPawn
            = Cast<ASlitherWormPawn>(CurrentPawn))
        {
            WormPawn->UpdateVisualState();
        }
    }

    UpdateLocalCamera(DeltaSeconds);
}
