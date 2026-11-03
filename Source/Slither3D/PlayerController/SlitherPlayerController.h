
#pragma once

#include "GameFramework/PlayerController.h"

#include "SlitherPlayerController.generated.h"

class USlitherPlayerUIComponent;
class USlitherPlayerCameraComponent;
class USlitherPlayerInputComponent;
class USlitherPlayerServerComponent;


UCLASS()
class ASlitherPlayerController : public APlayerController
{
    GENERATED_BODY()
private:

    UPROPERTY()
    TObjectPtr<USlitherPlayerCameraComponent> PlayerCameraComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherPlayerInputComponent> PlayerInputComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherPlayerServerComponent> PlayerServerComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherPlayerUIComponent> PlayerUiComponent = nullptr;

private:

    ASlitherPlayerController();

    void InitializeLocalPresentation() const;

protected:

    void BeginPlay() override;

public:

    void ReceivedPlayer() override;

    void PlayerTick(
        const float DeltaSeconds) override;

    void UpdateCameraManager(
        const float DeltaSeconds) override;


    UFUNCTION(BlueprintPure, Category = "SlitherUi")
    USlitherPlayerServerComponent* GetPlayerServerComponent() const;

    UFUNCTION(BlueprintPure, Category = "SlitherUi")
    USlitherPlayerUIComponent* GetPlayerUIComponent() const;
};
