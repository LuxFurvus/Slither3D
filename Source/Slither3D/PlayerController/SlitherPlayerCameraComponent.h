
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherPlayerCameraComponent.generated.h"


class ACameraActor;
class APlayerController;


UCLASS()
class USlitherPlayerCameraComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    bool CameraHasTrackedPawn = false;

    UPROPERTY()
    TObjectPtr<ACameraActor> LocalCamera = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Camera",
        meta = (ClampMin = "-90.0", ClampMax = "-0.001"))
    float CameraPitch = -75.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float CameraYaw = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera",
        meta = (ClampMin = "1.0"))
    float CameraDistance = 2000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera",
        meta = (ClampMin = "1.0", ClampMax = "179.0"))
    float CameraFieldOfView = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera",
        meta = (ClampMin = "0.001"))
    float CameraFollowSpeed = 6.0f;

private:

    ACameraActor* CreateLocalCamera();

    void UpdateLocalCamera(
        const float DeltaSeconds);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    USlitherPlayerCameraComponent();

    static void ConfigureController(
        APlayerController* PlayerController);

    void InitializeLocalCamera();

    void UpdateCamera(
        const float DeltaSeconds);
};
