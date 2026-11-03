
#include "SlitherPlayerServerComponent.h"

#include "SlitherPlayerController.h"
#include "SlitherPlayerUIComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

#include "Slither3D/WormPawn/SlitherWormPawn.h"
#include "Slither3D/GameMode/SlitherGameMode.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherPlayerServerComponent::USlitherPlayerServerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

bool USlitherPlayerServerComponent::ConsumeInputBudget(
    const double InputTime)
{
    constexpr double Capacity
        = FSlitherSettingsNetwork::InputBurstCapacity;

    if (InputBudgetTime < 0.0)
    {
        InputBudgetTime = InputTime;
        InputTokens = Capacity;
    }

    const double Elapsed = InputTime - InputBudgetTime;
    const double RefilledTokens
        = InputTokens + Elapsed
        * FSlitherSettingsNetwork::InputSendRate;

    InputTokens = FMath::Clamp(RefilledTokens, 0.0, Capacity);
    InputBudgetTime = InputTime;

    if (InputTokens < 1.0)
    {
        return false;
    }

    InputTokens -= 1.0;

    return true;
}

bool USlitherPlayerServerComponent::TryNormalizeAimDirection(
    const FVector& AimDirection,
    FVector& NormalizedDirection)
{
    if (const double AbsoluteZ
            = FMath::Abs(AimDirection.Z);
        AbsoluteZ > DirectionZTolerance)
    {
        return false;
    }

    constexpr double MinimumLengthSquared
        = MinimumDirectionLength * MinimumDirectionLength;
    constexpr double MaximumLengthSquared
        = MaximumDirectionLength * MaximumDirectionLength;
    const double LengthSquared
        = AimDirection.X * AimDirection.X + AimDirection.Y * AimDirection.Y;

    if (LengthSquared < MinimumLengthSquared
        || LengthSquared > MaximumLengthSquared)
    {
        return false;
    }

    NormalizedDirection = AimDirection;
    NormalizedDirection.Z = 0.0;
    const bool DirectionWasNormalized = NormalizedDirection.Normalize();

    return DirectionWasNormalized;
}

void USlitherPlayerServerComponent::ClientSessionUpdate_Implementation(
    const uint32 LifeId,
    const int32 Kills,
    const int32 Mass,
    const int32 TotalKillScore,
    const int32 TotalMassScore,
    const int32 Seconds,
    const bool SessionActive,
    const double RespawnAvailableServerTime)
{
    AActor* OwnerActor = GetOwner();

    const ASlitherPlayerController* PlayerController
        = Cast<ASlitherPlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    const bool LocalController
        = PlayerController->IsLocalController();

    if (!LocalController)
    {
        return;
    }

    USlitherPlayerUIComponent* PlayerUIComponent
        = PlayerController->GetPlayerUIComponent();

    if (!PlayerUIComponent)
    {
        return;
    }

    PlayerUIComponent->HandleSessionUpdate(
        LifeId,
        Kills,
        Mass,
        TotalKillScore,
        TotalMassScore,
        Seconds,
        SessionActive,
        RespawnAvailableServerTime);
}

void USlitherPlayerServerComponent::ClientStartGameRequestResult_Implementation(
    const uint32 ExpectedLifeId,
    const bool Accepted)
{
    AActor* OwnerActor = GetOwner();

    const ASlitherPlayerController* PlayerController
        = Cast<ASlitherPlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const bool LocalController
            = PlayerController->IsLocalController();
        !LocalController)
    {
        return;
    }

    USlitherPlayerUIComponent* PlayerUIComponent
        = PlayerController->GetPlayerUIComponent();

    if (PlayerUIComponent)
    {
        PlayerUIComponent->HandleStartGameRequestResult(
            ExpectedLifeId, Accepted);
    }
}

void USlitherPlayerServerComponent::ServerSubmitInput_Implementation(
    const uint32 LifeId,
    const uint32 Sequence,
    const FVector AimDirection,
    const bool WantsBoost)
{
    AActor* OwnerActor = GetOwner();
    const APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const bool HasServerAuthority
            = PlayerController->HasAuthority();
        !HasServerAuthority)
    {
        return;
    }

    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return;
    }

    const double InputTime = ThisWorld->GetTimeSeconds();

    if (const bool HasInputBudget
            = ConsumeInputBudget(InputTime);
        !HasInputBudget)
    {
        return;
    }

    APawn* CurrentPawn = PlayerController->GetPawn();
    const ASlitherWormPawn* WormPawn
        = Cast<ASlitherWormPawn>(CurrentPawn);

    if (!WormPawn)
    {
        return;
    }

    const AController* PawnController = WormPawn->GetController();
    const AActor* PawnOwner = WormPawn->GetOwner();
    const bool PawnIsBeingDestroyed = WormPawn->IsActorBeingDestroyed();
    const bool LifeActive = WormPawn->HasActiveLife();
    const uint32 CurrentLifeId = WormPawn->GetLifeId();

    const bool InputMustBeRejected
        = PawnController != PlayerController
        || PawnOwner != PlayerController
        || PawnIsBeingDestroyed
        || !LifeActive
        || CurrentLifeId == 0
        || LifeId != CurrentLifeId;

    if (InputMustBeRejected)
    {
        return;
    }

    FVector NormalizedDirection;
    const bool DirectionIsAcceptable
        = TryNormalizeAimDirection(AimDirection, NormalizedDirection);

    if (!DirectionIsAcceptable)
    {
        return;
    }

    (void) WormPawn->SubmitInput(
        LifeId, Sequence, NormalizedDirection, WantsBoost, InputTime);
}

void USlitherPlayerServerComponent::ServerRequestStartGame_Implementation(
    const uint32 ExpectedLifeId)
{
    AActor* OwnerActor = GetOwner();
    APlayerController* PlayerController
        = Cast<APlayerController>(OwnerActor);

    if (!PlayerController)
    {
        return;
    }

    if (const bool Authority
            = PlayerController->HasAuthority();
        !Authority)
    {
        return;
    }

    bool Accepted = false;

    if (const UWorld* ThisWorld = GetWorld())
    {
        ASlitherGameMode* SlitherGameMode
            = ThisWorld->GetAuthGameMode<ASlitherGameMode>();

        if (SlitherGameMode)
        {
            Accepted = SlitherGameMode->TryRequestStartGame(
                PlayerController, ExpectedLifeId);
        }
    }

    ClientStartGameRequestResult(
        ExpectedLifeId, Accepted);
}

void USlitherPlayerServerComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    InputBudgetTime = -1.0;
    InputTokens = 0.0;

    Super::EndPlay(EndPlayReason);
}
