
#pragma once

#include "GameFramework/HUD.h"

#include "SlitherHUD.generated.h"


UCLASS()
class ASlitherHUD : public AHUD
{
    GENERATED_BODY()

private:

    UPROPERTY(EditDefaultsOnly, Category = "SlitherUi")
    TSubclassOf<UUserWidget> RestartMenuClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "SlitherUi")
    TSubclassOf<UUserWidget> PlayerHudClass = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> RestartMenuWidget = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> PlayerHudWidget = nullptr;

private:

    void CloseWidgets();

    bool ShowWidget(
        const bool HudWidget);

protected:

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    bool ShowRestartMenu();

    bool ShowPlayerHud();

    UFUNCTION(BlueprintCallable, Category = "SlitherUi")
    static void SecondsToTime(
        const int32 InSeconds,
        int32& OutMinutes,
        int32& OutSeconds);
};
