
#include "SlitherWormSimulationComponent.h"

#include "SlitherFoodManagerComponent.h"
#include "SlitherWormCollisionComponent.h"
#include "SlitherWormSpawnComponent.h"
#include "GameFramework/Controller.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "Slither3D/WormPawn/SlitherBodyComponent.h"

#include "Engine/World.h"
#include "Misc/AssertionMacros.h"

#include "Slither3D/SlitherGameState.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherWormSimulationComponent::USlitherWormSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool USlitherWormSimulationComponent::CanUseStepState(
    const FSlitherWormStepState& State)
{
    const ASlitherWormPawn* WormPawn
        = State.WormPawn.Get();
    const ASlitherPlayerState* PlayerState
        = State.PlayerState.Get();

    if (!WormPawn || !PlayerState)
    {
        return false;
    }

    const bool LifeActive = WormPawn->HasActiveLife();
    const uint32 LifeId = WormPawn->GetLifeId();
    const ASlitherPlayerState* CurrentPlayerState =
        WormPawn->GetPlayerState<ASlitherPlayerState>();
    const AController* Controller = WormPawn->GetController();

    if (!LifeActive || LifeId != State.LifeId
        || CurrentPlayerState != PlayerState || !Controller)
    {
        return false;
    }

    const APawn* ControlledPawn = Controller->GetPawn();
    return ControlledPawn == WormPawn;
}

void USlitherWormSimulationComponent::ApplyDeaths(
    const ASlitherArena* Arena,
    USlitherFoodManagerComponent* FoodManager,
    USlitherWormSpawnComponent* SpawnManager,
    const double ServerTime)
{
    AcceptedDeaths.Reset();

    for (const FSlitherWormDeathRecord& Death : DeathRecords)
    {
        const ASlitherWormPawn* WormPawn
            = Death.WormPawn.Get();

        ASlitherPlayerState* PlayerState
            = Death.PlayerState.Get();

        if (!WormPawn || !PlayerState)
        {
            continue;
        }

        const bool LifeActive = WormPawn->HasActiveLife();
        const uint32 LifeId = WormPawn->GetLifeId();
        const ASlitherPlayerState* CurrentPlayerState =
            WormPawn->GetPlayerState<ASlitherPlayerState>();

        if (!LifeActive
            || LifeId != Death.LifeId
            || CurrentPlayerState != PlayerState)
        {
            continue;
        }

        if (const bool LifeEnded
            = PlayerState->TryEndLife(Death.LifeId))
        {
            AcceptedDeaths.Emplace(Death);
        }
    }

    for (FSlitherWormDeathRecord& Death : AcceptedDeaths)
    {
        ASlitherPlayerState* Killer = Death.KillerPlayerState.Get();
        const ASlitherPlayerState* Victim = Death.PlayerState.Get();

        if (Death.Reason != ESlitherDeathReason::Body || !Killer || Killer == Victim)
        {
            Death.KillerPlayerId = INDEX_NONE;
            continue;
        }

        const bool BeingDestroyed = Killer->IsActorBeingDestroyed();
        const bool Authority = Killer->HasAuthority();
        const uint32 LifeId = Killer->GetLifeId();

        if (!Authority || BeingDestroyed || LifeId != Death.KillerLifeId)
        {
            Death.KillerPlayerId = INDEX_NONE;
            continue;
        }

        Killer->AddKill(Death.KillerLifeId);
    }

    for (const FSlitherWormDeathRecord& Death : AcceptedDeaths)
    {
        if (ASlitherPlayerState* PlayerState
            = Death.PlayerState.Get())
        {
            PlayerState->FinalizeLifeDeath(
                Death.LifeId, ServerTime);
        }
    }

    for (const FSlitherWormDeathRecord& Death : AcceptedDeaths)
    {
        FoodManager->QueueDeathFood(Arena, Death);
    }

    for (const FSlitherWormDeathRecord& Death : AcceptedDeaths)
    {
        ASlitherWormPawn* WormPawn = Death.WormPawn.Get();

        if (!WormPawn)
        {
            continue;
        }

        WormPawn->ResetInputState();
        SpawnManager->UnregisterWormPawn(WormPawn);

        if (AController* Controller
            = WormPawn->GetController())
        {
            if (const APawn* ControlledPawn
                    = Controller->GetPawn();
                ControlledPawn == WormPawn)
            {
                Controller->UnPossess();
            }
        }

        WormPawn->Destroy();
    }
}

void USlitherWormSimulationComponent::SimulateStep(
    const ASlitherArena* Arena,
    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns,
    const double StepSeconds,
    const double ServerTime,
    USlitherFoodManagerComponent* FoodManager,
    USlitherWormCollisionComponent* CollisionManager,
    USlitherWormSpawnComponent* SpawnManager)
{
    if (!Arena || !FoodManager || !CollisionManager || !SpawnManager)
    {
        return;
    }

    ++SimulationFrame;
    StepStates.Reset();

    for (const TWeakObjectPtr<ASlitherWormPawn>& PawnReference : WormPawns)
    {
        ASlitherWormPawn* WormPawn = PawnReference.Get();

        if (!WormPawn)
        {
            continue;
        }

        FSlitherWormStepState State;
        State.WormPawn = WormPawn;
        State.PlayerState = WormPawn->GetPlayerState<ASlitherPlayerState>();
        State.LifeId = WormPawn->GetLifeId();
        const bool CanUse = CanUseStepState(State);

        if (!CanUse)
        {
            WormPawn->ResetInputState();
            continue;
        }

        const ASlitherPlayerState* PlayerState = State.PlayerState.Get();
        State.PlayerId = PlayerState->GetPlayerId();
        State.Mass = PlayerState->GetMass();
        State.PreviousHeadLocation = WormPawn->GetActorLocation();

        if (const USlitherBodyComponent* Body
            = WormPawn->GetBodyComponent())
        {
            State.PreviousSegmentLocations
                = Body->GetServerSegmentLocations();
        }

        StepStates.Emplace(State);
    }

    for (FSlitherWormStepState& State : StepStates)
    {
        if (const bool CanUse
                = CanUseStepState(State);
            !CanUse)
        {
            continue;
        }

        const ASlitherWormPawn* WormPawn = State.WormPawn.Get();
        WormPawn->SimulateMovement(Arena, StepSeconds, ServerTime, State.MovementResult);
        State.CurrentHeadLocation = State.MovementResult.CurrentHeadLocation;
        const FVector Movement = State.CurrentHeadLocation - State.PreviousHeadLocation;
        State.MovedDistance = Movement.Size2D();
    }

    int32 StateIndex = 0;
    int32 StateCount = StepStates.Num();

    while (StateIndex < StateCount)
    {
        FSlitherWormStepState& State = StepStates[StateIndex];

        if (const bool CanUse
                = CanUseStepState(State);
            !CanUse)
        {
            StepStates.RemoveAt(StateIndex, 1, EAllowShrinking::No);
            --StateCount;
            continue;
        }

        const ASlitherWormPawn* WormPawn = State.WormPawn.Get();

        if (USlitherBodyComponent* Body
            = WormPawn->GetBodyComponent())
        {
            Body->RefreshServerSegments();
            State.CurrentSegmentLocations
                = Body->GetServerSegmentLocations();
        }

        ++StateIndex;
    }

    FoodEntries = FoodManager->GatherCollisionEntries();

    CollisionManager->BuildCollisionGrid(
        StepStates, FoodEntries);
    CollisionManager->FindContacts(
        StepStates, FoodEntries, FoodContacts, DeathRecords);

    ApplyDeaths(Arena, FoodManager, SpawnManager, ServerTime);

    FoodManager->SpawnPendingDeathFood();
    FoodManager->CollectFood(FoodContacts);

    for (const FSlitherWormStepState& State : StepStates)
    {
        if (const bool CanUse
                = CanUseStepState(State);
            !CanUse)
        {
            continue;
        }

        const ASlitherWormPawn* WormPawn = State.WormPawn.Get();
        const ASlitherPlayerState* PlayerState = State.PlayerState.Get();

        if (USlitherBodyComponent* Body
            = WormPawn->GetBodyComponent())
        {
            const int32 Mass = PlayerState->GetMass();
            Body->ApplyGrowth(Mass, State.MovedDistance);
        }
    }

    FoodManager->ReplenishNaturalFood(Arena, WormPawns);
}

void USlitherWormSimulationComponent::PublishSnapshots(
    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns) const
{
    for (const TWeakObjectPtr<ASlitherWormPawn>& PawnReference : WormPawns)
    {
        if (const ASlitherWormPawn* WormPawn = PawnReference.Get())
        {
            if (const bool LifeActive
                = WormPawn->HasActiveLife())
            {
                WormPawn->PublishSnapshot(SimulationFrame, LastSimulationTime);
            }
        }
    }
}

void USlitherWormSimulationComponent::UpdateSimulation(
    const float DeltaSeconds,
    const ASlitherArena* Arena,
    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns,
    USlitherFoodManagerComponent* FoodManager,
    USlitherWormCollisionComponent* CollisionManager,
    USlitherWormSpawnComponent* SpawnManager)
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return;
    }

    const bool Authority
        = OwnerActor->HasAuthority();

    if (!Authority || !Arena || !FoodManager || !CollisionManager
        || !SpawnManager || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const UWorld* ThisWorld = GetWorld();
    check(ThisWorld);

    const ASlitherGameState* SlitherGameState
        = ThisWorld->GetGameState<ASlitherGameState>();

    if (!SlitherGameState)
    {
        return;
    }

    const double ServerNow = SlitherGameState->GetServerWorldTimeSeconds();
    constexpr double StepSeconds = FSlitherSettingsNetwork::ServerSimulationInterval;
    constexpr int32 MaxSubsteps = FSlitherSettingsNetwork::MaxCatchUpSubsteps;
    constexpr double MaximumDebt = StepSeconds * MaxSubsteps;
    SimulationAccumulator += DeltaSeconds;
    PublishAccumulator += DeltaSeconds;

    if (SimulationAccumulator >= MaximumDebt + StepSeconds)
    {
        const double Remainder = FMath::Fmod(SimulationAccumulator, StepSeconds);
        SimulationAccumulator = MaximumDebt + Remainder;
    }

    int32 SubstepCount = 0;

    while (SimulationAccumulator >= StepSeconds && SubstepCount < MaxSubsteps)
    {
        SimulationAccumulator -= StepSeconds;
        LastSimulationTime = ServerNow - SimulationAccumulator;
        SimulateStep(
            Arena, WormPawns, StepSeconds, ServerNow,
            FoodManager, CollisionManager, SpawnManager);
        ++SubstepCount;
    }

    if (constexpr double PublishInterval
            = FSlitherSettingsNetwork::WormStatePublishInterval;
        SubstepCount > 0 && PublishAccumulator >= PublishInterval)
    {
        PublishAccumulator = FMath::Fmod(PublishAccumulator, PublishInterval);
        PublishSnapshots(WormPawns);
    }
}

uint32 USlitherWormSimulationComponent::GetSimulationFrame() const
{
    return SimulationFrame;
}
