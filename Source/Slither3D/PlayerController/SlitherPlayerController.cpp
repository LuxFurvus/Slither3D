
#include "SlitherPlayerController.h"

#include "SlitherPlayerCameraComponent.h"
#include "SlitherPlayerInputComponent.h"
#include "SlitherPlayerServerComponent.h"
#include "SlitherPlayerUIComponent.h"


ASlitherPlayerController::ASlitherPlayerController()
{
    USlitherPlayerCameraComponent::ConfigureController(this);

    PlayerCameraComponent
        = CreateDefaultSubobject<USlitherPlayerCameraComponent>(
            TEXT("PlayerCameraComponent"));

    PlayerInputComponent
        = CreateDefaultSubobject<USlitherPlayerInputComponent>(
            TEXT("PlayerInputComponent"));

    PlayerServerComponent
        = CreateDefaultSubobject<USlitherPlayerServerComponent>(
            TEXT("PlayerServerComponent"));

    PlayerUiComponent
        = CreateDefaultSubobject<USlitherPlayerUIComponent>(
            TEXT("PlayerUiComponent"));
}

void ASlitherPlayerController::InitializeLocalPresentation() const
{
    if (const ENetMode NetMode = GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return;
    }

    if (const bool ControllerIsLocal = IsLocalController();
        !ControllerIsLocal)
    {
        return;
    }

    PlayerInputComponent->ConfigureLocalInput();

    PlayerCameraComponent->InitializeLocalCamera();

    PlayerUiComponent->UpdateLocalPresentation();
}

void ASlitherPlayerController::BeginPlay()
{
    Super::BeginPlay();

    InitializeLocalPresentation();
}

void ASlitherPlayerController::PlayerTick(
    const float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);

    PlayerInputComponent->UpdateLocalInput(
        DeltaSeconds, PlayerServerComponent);

    PlayerUiComponent->UpdateLocalPresentation();
}

void ASlitherPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();

    InitializeLocalPresentation();
}

void ASlitherPlayerController::UpdateCameraManager(
    const float DeltaSeconds)
{
    PlayerCameraComponent->UpdateCamera(DeltaSeconds);

    Super::UpdateCameraManager(DeltaSeconds);
}

USlitherPlayerServerComponent* ASlitherPlayerController::GetPlayerServerComponent() const
{
    return PlayerServerComponent;
}

USlitherPlayerUIComponent* ASlitherPlayerController::GetPlayerUIComponent() const
{
    return PlayerUiComponent;
}
