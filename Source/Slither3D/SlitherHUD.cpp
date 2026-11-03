
#include "SlitherHUD.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"


void ASlitherHUD::CloseWidgets()
{
    if (RestartMenuWidget)
    {
        RestartMenuWidget->RemoveFromParent();
        RestartMenuWidget = nullptr;
    }

    if (PlayerHudWidget)
    {
        PlayerHudWidget->RemoveFromParent();
        PlayerHudWidget = nullptr;
    }
}

bool ASlitherHUD::ShowWidget(
    const bool HudWidget)
{
    APlayerController* PlayerController
        = GetOwningPlayerController();

    if (!PlayerController)
    {
        return false;
    }

    if (const bool LocalController
            = PlayerController->IsLocalController();
        !LocalController)
    {
        return false;
    }

    const TSubclassOf<UUserWidget> WidgetClass
        = HudWidget ? PlayerHudClass : RestartMenuClass;

    if (!WidgetClass)
    {
        return false;
    }

    TObjectPtr<UUserWidget>& Widget
        = HudWidget ? PlayerHudWidget : RestartMenuWidget;

    if (Widget)
    {
        if (const bool Visible
            = Widget->IsInViewport())
        {
            return true;
        }
    }

    CloseWidgets();

    Widget = CreateWidget<UUserWidget>(
        PlayerController,
        WidgetClass,
        NAME_None);

    if (!Widget)
    {
        return false;
    }

    const bool Added
        = Widget->AddToPlayerScreen(0);

    if (!Added)
    {
        CloseWidgets();
    }
    else if (HudWidget)
    {
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }

    return Added;
}

bool ASlitherHUD::ShowRestartMenu()
{
    const bool Shown = ShowWidget(false);

    return Shown;
}

bool ASlitherHUD::ShowPlayerHud()
{
    const bool Shown = ShowWidget(true);

    return Shown;
}

void ASlitherHUD::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    CloseWidgets();

    Super::EndPlay(EndPlayReason);
}

void ASlitherHUD::SecondsToTime(
    const int32 InSeconds,
    int32& OutMinutes,
    int32& OutSeconds)
{
    OutMinutes = InSeconds / 60;
    OutSeconds = InSeconds % 60;
}
