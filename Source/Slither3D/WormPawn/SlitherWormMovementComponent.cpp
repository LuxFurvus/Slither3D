
#include "SlitherWormMovementComponent.h"

#include "SlitherWormPawn.h"
#include "Slither3D/SlitherArena.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherWormMovementComponent::USlitherWormMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

ASlitherWormPawn* USlitherWormMovementComponent::GetWormPawn() const
{
    AActor* OwnerActor = GetOwner();

    ASlitherWormPawn* WormPawn
        = Cast<ASlitherWormPawn>(OwnerActor);
    check(WormPawn);

    return WormPawn;
}

void USlitherWormMovementComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ResetInputState();

    Super::EndPlay(EndPlayReason);
}

const FSlitherWormInputState& USlitherWormMovementComponent::GetInputState() const
{
    return InputState;
}

float USlitherWormMovementComponent::GetCurrentSpeed() const
{
    return CurrentSpeed;
}

void USlitherWormMovementComponent::ResetInputState()
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();

    if (!HasServerAuthority)
    {
        return;
    }

    const FSlitherWormInputState InitialInputState;
    InputState = InitialInputState;
    InputState.AimDirection = WormPawn->GetActorForwardVector();
}

bool USlitherWormMovementComponent::SubmitInput(
    const uint32 InputLifeId,
    const uint32 InputSequence,
    const FVector& AimDirection,
    const bool WantsBoost,
    const double InputTime)
{
    const ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();
    const uint32 LifeId = WormPawn->GetLifeId();

    const bool LifeActive = WormPawn->HasActiveLife();

    if (!HasServerAuthority
        || !LifeActive
        || LifeId == 0
        || InputLifeId != LifeId)
    {
        return false;
    }

    if (InputState.HasInput)
    {
        const uint32 SequenceDelta
            = InputSequence - InputState.LastInputSequence;

        if (SequenceDelta == 0
            || SequenceDelta > MaxForwardSequenceDelta)
        {
            return false;
        }
    }

    InputState.AimDirection = AimDirection;
    InputState.WantsBoost = WantsBoost;
    InputState.LastInputSequence = InputSequence;
    InputState.LastInputTime = InputTime;
    InputState.HasInput = true;

    return true;
}

void USlitherWormMovementComponent::SimulateMovement(
    const ASlitherArena* Arena,
    const double StepSeconds,
    const double ServerTime,
    FSlitherWormMovementResult& OutResult)
{
    const FSlitherWormMovementResult InitialResult;
    OutResult = InitialResult;
    ASlitherWormPawn* WormPawn = GetWormPawn();
    const bool HasServerAuthority = WormPawn->HasAuthority();
    const uint32 LifeId = WormPawn->GetLifeId();

    const bool LifeActive = WormPawn->HasActiveLife();
    const bool CanSimulateMovement
        = HasServerAuthority
        && LifeActive
        && Arena
        && LifeId != 0
        && StepSeconds > 0.0;

    if (!CanSimulateMovement)
    {
        return;
    }

    const double InputAge = ServerTime - InputState.LastInputTime;
    const bool InputIsFresh = InputState.HasInput
        && InputAge >= 0.0
        && InputAge <= FSlitherSettingsNetwork::InputTimeout;

    if (!InputIsFresh)
    {
        InputState.WantsBoost = false;
    }

    const float Speed = InputState.WantsBoost
        ? FSlitherSettingsMovement::BoostSpeed
        : FSlitherSettingsMovement::BaseSpeed;
    const FRotator CurrentRotation = WormPawn->GetActorRotation();
    const FRotator TargetRotation = InputState.AimDirection.Rotation();
    double YawDelta = FMath::FindDeltaAngleDegrees(
        CurrentRotation.Yaw, TargetRotation.Yaw);

    if (YawDelta <= -180.0)
    {
        YawDelta = 180.0;
    }

    const double MaxYawStep = FSlitherSettingsMovement::MaxTurnRate * StepSeconds;
    const double AppliedYawDelta = FMath::Clamp(YawDelta, -MaxYawStep, MaxYawStep);
    const double NewYaw = CurrentRotation.Yaw + AppliedYawDelta;
    const FRotator NewRotation(0.0, NewYaw, 0.0);
    const FVector NewDirection = NewRotation.Vector();
    const double MoveDistance = Speed * StepSeconds;
    const FVector MoveDelta = NewDirection * MoveDistance;
    const FVector OldLocation = WormPawn->GetActorLocation();
    const FBox2D Bounds = Arena->GetPlayableBounds();
    constexpr double HeadRadius = FSlitherSettingsBody::HeadRadius;
    const double MinimumX = Bounds.Min.X + HeadRadius;
    const double MaximumX = Bounds.Max.X - HeadRadius;
    const double MinimumY = Bounds.Min.Y + HeadRadius;
    const double MaximumY = Bounds.Max.Y - HeadRadius;
    double MoveFraction = 1.0;
    const FVector IntendedLocation = OldLocation + MoveDelta;
    OutResult.PreviousHeadLocation = OldLocation;
    OutResult.IntendedHeadLocation = IntendedLocation;
    OutResult.HitWall = OldLocation.X <= MinimumX || OldLocation.X >= MaximumX
        || OldLocation.Y <= MinimumY || OldLocation.Y >= MaximumY
        || IntendedLocation.X <= MinimumX || IntendedLocation.X >= MaximumX
        || IntendedLocation.Y <= MinimumY || IntendedLocation.Y >= MaximumY;

    if (OldLocation.X <= MinimumX || OldLocation.X >= MaximumX
        || OldLocation.Y <= MinimumY || OldLocation.Y >= MaximumY)
    {
        MoveFraction = 0.0;
    }

    if (MoveDelta.X > 0.0)
    {
        const double WallFraction = (MaximumX - OldLocation.X) / MoveDelta.X;
        MoveFraction = FMath::Min(MoveFraction, WallFraction);
    }
    else if (MoveDelta.X < 0.0)
    {
        const double WallFraction = (MinimumX - OldLocation.X) / MoveDelta.X;
        MoveFraction = FMath::Min(MoveFraction, WallFraction);
    }

    if (MoveDelta.Y > 0.0)
    {
        const double WallFraction = (MaximumY - OldLocation.Y) / MoveDelta.Y;
        MoveFraction = FMath::Min(MoveFraction, WallFraction);
    }
    else if (MoveDelta.Y < 0.0)
    {
        const double WallFraction = (MinimumY - OldLocation.Y) / MoveDelta.Y;
        MoveFraction = FMath::Min(MoveFraction, WallFraction);
    }

    MoveFraction = FMath::Clamp(MoveFraction, 0.0, 1.0);
    const FVector AppliedDelta = MoveDelta * MoveFraction;
    FVector NewLocation = OldLocation + AppliedDelta;
    NewLocation.X = FMath::Clamp(NewLocation.X, MinimumX, MaximumX);
    NewLocation.Y = FMath::Clamp(NewLocation.Y, MinimumY, MaximumY);
    NewLocation.Z = Arena->GetWormHeadPlaneZ();
    OutResult.CurrentHeadLocation = NewLocation;
    OutResult.WallContactAlpha = MoveFraction;
    const double AppliedDistance = AppliedDelta.Size();
    CurrentSpeed = static_cast<float>(AppliedDistance / StepSeconds);

    WormPawn->SetActorLocationAndRotation(
        NewLocation, NewRotation, false, nullptr, ETeleportType::None);
}

