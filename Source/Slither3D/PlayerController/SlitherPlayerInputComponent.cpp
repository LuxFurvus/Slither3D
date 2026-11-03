
#include "SlitherPlayerInputComponent.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Math/UnrealMathUtility.h"
#include "UnrealClient.h"

#include "Slither3D/SlitherArena.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "SlitherPlayerServerComponent.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherPlayerInputComponent::USlitherPlayerInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void USlitherPlayerInputComponent::ConfigureLocalInput() const
{
    AActor* OwnerActor = GetOwner();
    APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    PlayerController->SetShowMouseCursor(true);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(
        EMouseLockMode::DoNotLock);

    PlayerController->SetInputMode(InputMode);

    UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

bool USlitherPlayerInputComponent::HasLocalInputFocus() const
{
    const AActor* OwnerActor = GetOwner();
    const APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return false;
    }

    const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

    if (!LocalPlayer)
    {
        return false;
    }

    const UGameViewportClient* ViewportClient
        = LocalPlayer->ViewportClient;

    if (!ViewportClient || !ViewportClient->Viewport)
    {
        return false;
    }

    const FViewport* Viewport = ViewportClient->Viewport;
    const bool ViewportHasFocus = Viewport->HasFocus();
    const bool WindowIsForeground = Viewport->IsForegroundWindow();

    return ViewportHasFocus && WindowIsForeground;
}

void USlitherPlayerInputComponent::ResetLocalInput()
{
    InputPawn.Reset();
    InputLifeId = 0;
    InputSequence = 0;
    InputSendAccumulator = 0.0;
    LastAimDirection = FVector::ForwardVector;
    BoostRequiresRelease = true;
    LocalWantsBoost = false;
}

ASlitherArena* USlitherPlayerInputComponent::FindInputArena()
{
    if (ASlitherArena* ArenaPointer
        = InputArena.Get())
    {
        return ArenaPointer;
    }

    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return nullptr;
    }

    ASlitherArena* FoundArena = nullptr;
    int32 ArenaCount = 0;

    for (TActorIterator<ASlitherArena> ArenaIterator(ThisWorld);
        ArenaIterator; ++ArenaIterator)
    {
        ASlitherArena* CandidateArena = *ArenaIterator;

        if (!CandidateArena)
        {
            continue;
        }

        const bool ArenaIsBeingDestroyed
            = CandidateArena->IsActorBeingDestroyed();

        if (ArenaIsBeingDestroyed)
        {
            continue;
        }

        FoundArena = CandidateArena;
        ++ArenaCount;
    }

    if (ArenaCount != 1)
    {
        return nullptr;
    }

    InputArena = FoundArena;

    return FoundArena;
}

void USlitherPlayerInputComponent::UpdateLocalAim(
    const ASlitherWormPawn* WormPawn)
{
    AActor* OwnerActor = GetOwner();
    const APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    const ASlitherArena* ArenaPointer
        = FindInputArena();

    if (!ArenaPointer)
    {
        return;
    }

    FVector RayOrigin;
    FVector RayDirection;
    const bool RayWasDeprojected
        = PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection);

    if (!RayWasDeprojected)
    {
        return;
    }

    const FPlane GameplayPlane = ArenaPointer->GetWormMovementPlane();
    const FVector PlaneNormal = GameplayPlane.GetNormal();
    const double RayDotNormal = FVector::DotProduct(RayDirection, PlaneNormal);

    if (const double AbsoluteRayDotNormal
            = FMath::Abs(RayDotNormal);
        AbsoluteRayDotNormal <= RayParallelTolerance)
    {
        return;
    }

    const double OriginPlaneDistance = GameplayPlane.PlaneDot(RayOrigin);
    const double RayDistance = -OriginPlaneDistance / RayDotNormal;

    if (RayDistance < 0.0)
    {
        return;
    }

    const FVector RayOffset = RayDirection * RayDistance;
    const FVector Intersection = RayOrigin + RayOffset;
    const FVector HeadLocation = WormPawn->GetVisualHeadLocation();
    FVector AimDirection = Intersection - HeadLocation;
    AimDirection.Z = 0.0;

    constexpr double AimDeadZoneSquared
        = FSlitherSettingsMovement::AimDeadZone
        * FSlitherSettingsMovement::AimDeadZone;

    if (const double DirectionLengthSquared
            = AimDirection.SizeSquared();
        DirectionLengthSquared < AimDeadZoneSquared)
    {
        return;
    }

    if (const bool DirectionIsNormalized
            = AimDirection.Normalize();
        !DirectionIsNormalized)
    {
        return;
    }

    FVector NormalizedDirection;
    const bool DirectionIsAcceptable
        = USlitherPlayerServerComponent::TryNormalizeAimDirection(
            AimDirection, NormalizedDirection);

    if (DirectionIsAcceptable)
    {
        LastAimDirection = NormalizedDirection;
    }
}

void USlitherPlayerInputComponent::UpdateLocalInput(
    const float DeltaSeconds,
    USlitherPlayerServerComponent* ServerComponent)
{
    AActor* OwnerActor = GetOwner();
    const APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController || !ServerComponent)
    {
        return;
    }

    if (const bool ControllerIsLocal
            = PlayerController->IsLocalController();
        !ControllerIsLocal)
    {
        return;
    }

    APawn* CurrentPawn = PlayerController->GetPawn();
    ASlitherWormPawn* WormPawn = Cast<ASlitherWormPawn>(CurrentPawn);

    if (!WormPawn)
    {
        ResetLocalInput();
        return;
    }

    const bool PawnIsBeingDestroyed = WormPawn->IsActorBeingDestroyed();
    const uint32 CurrentLifeId = WormPawn->GetLifeId();

    const ASlitherPlayerState* SlitherPlayerState =
        PlayerController->GetPlayerState<ASlitherPlayerState>();

    if (!SlitherPlayerState)
    {
        ResetLocalInput();
        return;
    }

    const uint32 PlayerLifeId = SlitherPlayerState->GetLifeId();
    const bool Alive = SlitherPlayerState->IsAlive();

    if (PawnIsBeingDestroyed || CurrentLifeId == 0
        || !Alive || PlayerLifeId != CurrentLifeId)
    {
        ResetLocalInput();
        return;
    }

    if (const ASlitherWormPawn* PreviousPawn
            = InputPawn.Get();
        PreviousPawn != WormPawn || InputLifeId != CurrentLifeId)
    {
        ResetLocalInput();
        InputPawn = WormPawn;
        InputLifeId = CurrentLifeId;
        LastAimDirection = WormPawn->GetActorForwardVector();
        LastAimDirection.Z = 0.0;
        const bool DirectionWasNormalized = LastAimDirection.Normalize();

        if (!DirectionWasNormalized)
        {
            LastAimDirection = FVector::ForwardVector;
        }

        InputSendAccumulator = FSlitherSettingsNetwork::InputSendInterval;
    }
    else
    {
        if (DeltaSeconds > 0.0f)
        {
            InputSendAccumulator += DeltaSeconds;
        }
    }

    const bool InputHasFocus = HasLocalInputFocus();
    const bool MouseIsDown
        = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);

    if (!InputHasFocus)
    {
        BoostRequiresRelease = true;
        LocalWantsBoost = false;
    }
    else
    {
        if (!MouseIsDown)
        {
            BoostRequiresRelease = false;
        }

        LocalWantsBoost = MouseIsDown && !BoostRequiresRelease;
        UpdateLocalAim(WormPawn);
    }

    constexpr double SendInterval = FSlitherSettingsNetwork::InputSendInterval;

    if (InputSendAccumulator < SendInterval)
    {
        return;
    }

    InputSendAccumulator = FMath::Fmod(InputSendAccumulator, SendInterval);
    ++InputSequence;

    ServerComponent->ServerSubmitInput(
        InputLifeId, InputSequence, LastAimDirection, LocalWantsBoost);
}

void USlitherPlayerInputComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ResetLocalInput();
    InputArena.Reset();

    Super::EndPlay(EndPlayReason);
}
