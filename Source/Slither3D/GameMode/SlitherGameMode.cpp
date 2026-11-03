
#include "SlitherGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Misc/AssertionMacros.h"

#include "SlitherFoodManagerComponent.h"
#include "SlitherWormSimulationComponent.h"
#include "SlitherWormCollisionComponent.h"
#include "SlitherWormSpawnComponent.h"
#include "Slither3D/Slither3D.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"
#include "Slither3D/SlitherArena.h"
#include "Slither3D/SlitherGameState.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"
#include "Slither3D/PlayerController/SlitherPlayerController.h"


ASlitherGameMode::ASlitherGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    GameStateClass
        = ASlitherGameState::StaticClass();
    PlayerStateClass
        = ASlitherPlayerState::StaticClass();
    PlayerControllerClass
        = ASlitherPlayerController::StaticClass();
    DefaultPawnClass
        = ASlitherWormPawn::StaticClass();

    WormSimulationComponent
        = CreateDefaultSubobject<USlitherWormSimulationComponent>(
            TEXT("WormSimulationComponent"));

    WormSpawnComponent
        = CreateDefaultSubobject<USlitherWormSpawnComponent>(
            TEXT("WormSpawnComponent"));

    FoodManagerComponent = CreateDefaultSubobject<USlitherFoodManagerComponent>(
        TEXT("FoodManagerComponent"));

    WormCollisionComponent = CreateDefaultSubobject<USlitherWormCollisionComponent>(
        TEXT("WormCollisionComponent"));
}

ASlitherArena* ASlitherGameMode::GetSingletonArena() const
{
    const UWorld* ThisWorld = GetWorld();
    check(ThisWorld);

    UClass* ArenaClass = ASlitherArena::StaticClass();
    TArray<AActor*> FoundActors;

    UGameplayStatics::GetAllActorsOfClass(
        ThisWorld, ArenaClass, FoundActors);

    const int32 ArenaCount = FoundActors.Num();
    checkf(ArenaCount == 1,
        TEXT("There must be only one arena actor in this world!"));

    AActor* FoundActor
        = FoundActors[0];

    ASlitherArena* FoundArena
        = Cast<ASlitherArena>(FoundActor);

    check(FoundArena);

    return FoundArena;
}

void ASlitherGameMode::CheckArenaRotationAndScale(
    const AActor* InArenaActor)
{
    constexpr double TransformTolerance = 0.0001;

    const FQuat ArenaRotation = InArenaActor->GetActorQuat();
    const bool RotationIsIdentity
        = ArenaRotation.Equals(FQuat::Identity, TransformTolerance);

    const FVector ArenaScale = InArenaActor->GetActorScale3D();
    const bool ScaleIsUnit
        = ArenaScale.Equals(FVector::OneVector, TransformTolerance);

    const bool bAreRotationAndScaleCorrect
        = RotationIsIdentity
        && ScaleIsUnit;

    checkf(bAreRotationAndScaleCorrect,
        TEXT("Arena must have identity rotation and unit scale."));
}

void ASlitherGameMode::InitializeArena()
{
    const bool HasServerAuthority
        = HasAuthority();
    check(HasServerAuthority);

    if (ArenaInitializationAttempted)
    {
        return;
    }

    check(ArenaActor == nullptr);
    ArenaInitializationAttempted = true;

    ASlitherArena* FoundArena
        = GetSingletonArena();

    CheckArenaRotationAndScale(FoundArena);

    ArenaActor = FoundArena;
}

void ASlitherGameMode::StartPlay()
{
    InitializeArena();

    check(ArenaActor);

    Super::StartPlay();

    AActor::SetActorTickEnabled(true);

    if (const UWorld* ThisWorld = GetWorld())
    {
        ASlitherGameState* SlitherGameState =
            ThisWorld->GetGameState<ASlitherGameState>();

        if (SlitherGameState)
        {
            SlitherGameState->MarkWorldReady();
        }
    }
}

void ASlitherGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (const bool HasServerAuthority = HasAuthority();
        !HasServerAuthority || !ArenaActor || DeltaSeconds <= 0.0f)
    {
        return;
    }

    if (const bool ArenaIsBeingDestroyed
        = ArenaActor->IsActorBeingDestroyed())
    {
        SetActorTickEnabled(false);
        return;
    }

    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns
        = WormSpawnComponent->GetWormPawns();
    WormSimulationComponent->UpdateSimulation(
        DeltaSeconds, ArenaActor, WormPawns, FoodManagerComponent,
        WormCollisionComponent, WormSpawnComponent);

    if (const double ServerTime
            = GetSpawnServerTime();
        ServerTime >= 0.0)
    {
        ProcessSpawnRequests(ServerTime);
    }
}

double ASlitherGameMode::GetSpawnServerTime() const
{
    const UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return -1.0;
    }

    const ASlitherGameState* SlitherGameState =
        ThisWorld->GetGameState<ASlitherGameState>();

    if (!SlitherGameState)
    {
        return -1.0;
    }

    const double ServerTime = SlitherGameState->GetServerWorldTimeSeconds();
    return ServerTime >= 0.0 ? ServerTime : -1.0;
}

ASlitherPlayerState* ASlitherGameMode::GetSpawnPlayerState(
    const AController* InController) const
{
    if (const bool Authority = HasAuthority();
        !Authority || EndingPlay || !InController)
    {
        return nullptr;
    }

    const bool BeingDestroyed = InController->IsActorBeingDestroyed();
    const bool IsPlayer = InController->IsPlayerController();
    const UWorld* ControllerWorld = InController->GetWorld();
    const UWorld* ThisWorld = GetWorld();

    if (BeingDestroyed || !IsPlayer || ControllerWorld != ThisWorld)
    {
        return nullptr;
    }

    ASlitherPlayerState* PlayerState =
        InController->GetPlayerState<ASlitherPlayerState>();

    if (!PlayerState)
    {
        return nullptr;
    }

    const bool StateBeingDestroyed = PlayerState->IsActorBeingDestroyed();
    const AActor* StateOwner = PlayerState->GetOwner();

    if (StateBeingDestroyed || StateOwner != InController)
    {
        return nullptr;
    }

    return PlayerState;
}

int32 ASlitherGameMode::FindSpawnRequest(const AController* Controller) const
{
    const int32 RequestCount = SpawnRequests.Num();

    for (int32 Index = 0; Index < RequestCount; ++Index)
    {
        const AController* RequestController = SpawnRequests[Index].Controller.Get();

        if (RequestController == Controller)
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

bool ASlitherGameMode::IsSpawnRequestCurrent(
    const FSlitherSpawnRequest& Request) const
{
    const AController* Controller = Request.Controller.Get();
    const ASlitherPlayerState* ExpectedState = Request.PlayerState.Get();

    if (!Controller || !ExpectedState || EndingPlay)
    {
        return false;
    }

    const ASlitherPlayerState* CurrentState = GetSpawnPlayerState(Controller);
    const int32 Index = FindSpawnRequest(Controller);

    if (CurrentState != ExpectedState || Index == INDEX_NONE)
    {
        return false;
    }

    const uint32 LifeId = CurrentState->GetLifeId();
    const bool Waiting = CurrentState->IsWaitingForSpawn();
    const FSlitherSpawnRequest& CurrentRequest = SpawnRequests[Index];

    return Waiting && LifeId == Request.ExpectedLifeId
        && CurrentRequest.PlayerState == Request.PlayerState
        && CurrentRequest.ExpectedLifeId == Request.ExpectedLifeId;
}

bool ASlitherGameMode::CompareSpawnRequests(
    const FSlitherSpawnRequest& First,
    const FSlitherSpawnRequest& Second)
{
    const ASlitherPlayerState* FirstState = First.PlayerState.Get();
    const ASlitherPlayerState* SecondState = Second.PlayerState.Get();
    const int32 FirstId = FirstState ? FirstState->GetPlayerId() : INDEX_NONE;
    const int32 SecondId = SecondState ? SecondState->GetPlayerId() : INDEX_NONE;
    return FirstId < SecondId;
}

void ASlitherGameMode::DiscardSpawnedPawn(
    AController* Controller,
    APawn* SpawnedPawn)
{
    if (!SpawnedPawn)
    {
        return;
    }

    if (Controller)
    {
        const APawn* ControlledPawn = Controller->GetPawn();

        if (ControlledPawn == SpawnedPawn)
        {
            Controller->UnPossess();
            const APawn* RemainingPawn = Controller->GetPawn();

            if (RemainingPawn == SpawnedPawn)
            {
                Controller->SetPawn(nullptr);
            }
        }
    }

    const bool BeingDestroyed = SpawnedPawn->IsActorBeingDestroyed();

    if (!BeingDestroyed)
    {
        SpawnedPawn->Destroy();
    }
}

bool ASlitherGameMode::TrySpawnRequest(const FSlitherSpawnRequest& Request)
{
    const bool Current = IsSpawnRequestCurrent(Request);

    if (!Current || !ArenaActor)
    {
        return false;
    }

    AController* Controller = Request.Controller.Get();
    ASlitherPlayerState* PlayerState = Request.PlayerState.Get();

    if (const APawn* ExistingPawn
        = Controller->GetPawn())
    {
        return false;
    }

    const TOptional<FTransform> Candidate =
        WormSpawnComponent->TryGetSpawnTransform(ArenaActor);

    if (const bool Found
            = Candidate.IsSet();
        !Found)
    {
        return false;
    }

    const FTransform SpawnTransform = Candidate.GetValue();
    APawn* SpawnedPawn = SpawnDefaultPawnAtTransform(Controller, SpawnTransform);
    ASlitherWormPawn* WormPawn = Cast<ASlitherWormPawn>(SpawnedPawn);
    const bool CurrentAfterCreation = IsSpawnRequestCurrent(Request);

    if (!WormPawn || !CurrentAfterCreation)
    {
        UE_LOG(SlitherLog, Warning, TEXT("Worm creation failed or its spawn request expired."));
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    const bool DestroyedAfterCreation = WormPawn->IsActorBeingDestroyed();
    const APawn* PawnAfterCreation = Controller->GetPawn();

    if (DestroyedAfterCreation || PawnAfterCreation)
    {
        UE_LOG(SlitherLog, Warning, TEXT("Worm spawn was interrupted during creation."));
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    Controller->SetPawn(SpawnedPawn);
    const bool CurrentAfterAssignment = IsSpawnRequestCurrent(Request);
    const bool DestroyedAfterAssignment = WormPawn->IsActorBeingDestroyed();
    const APawn* AssignedPawn = Controller->GetPawn();

    if (!CurrentAfterAssignment || DestroyedAfterAssignment
        || AssignedPawn != SpawnedPawn)
    {
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    const FRotator SpawnRotation = SpawnTransform.Rotator();
    FinishRestartPlayer(Controller, SpawnRotation);
    const bool CurrentAfterPossession = IsSpawnRequestCurrent(Request);
    const bool DestroyedAfterPossession = WormPawn->IsActorBeingDestroyed();

    if (!CurrentAfterPossession || DestroyedAfterPossession)
    {
        UE_LOG(SlitherLog, Warning, TEXT("Worm spawn request expired during possession."));
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    const APawn* PossessedPawn = Controller->GetPawn();
    const AController* PawnController = WormPawn->GetController();
    const AActor* PawnOwner = WormPawn->GetOwner();
    const ASlitherPlayerState* PawnPlayerState =
        WormPawn->GetPlayerState<ASlitherPlayerState>();
    const FTransform ActualTransform = WormPawn->GetActorTransform();
    const bool TransformMatches = ActualTransform.Equals(SpawnTransform, 0.0001);

    if (PossessedPawn != WormPawn || PawnController != Controller
        || PawnOwner != Controller || PawnPlayerState != PlayerState
        || !TransformMatches)
    {
        UE_LOG(SlitherLog, Warning, TEXT("Worm possession or spawn transform did not match."));
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    const uint32 NewLifeId = PlayerState->CalculateNextLifeId();
    const bool Prepared = WormPawn->InitializeLife(NewLifeId);
    const bool CurrentAfterPreparation = IsSpawnRequestCurrent(Request);
    const double SpawnServerTime = GetSpawnServerTime();

    if (!Prepared || !CurrentAfterPreparation || SpawnServerTime < 0.0)
    {
        UE_LOG(SlitherLog, Warning, TEXT("Worm life preparation failed."));
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    const bool BeganLife = PlayerState->TryBeginNextLife(
        Request.ExpectedLifeId, SpawnServerTime);

    if (!BeganLife)
    {
        DiscardSpawnedPawn(Controller, SpawnedPawn);
        return false;
    }

    WormSpawnComponent->RegisterWormPawn(WormPawn);
    const int32 RequestIndex = FindSpawnRequest(Controller);
    SpawnRequests.RemoveAt(RequestIndex, 1, EAllowShrinking::No);
    const uint32 SimulationFrame = WormSimulationComponent->GetSimulationFrame();
    WormPawn->PublishSnapshot(SimulationFrame, SpawnServerTime);
    WormPawn->ForceNetUpdate();
    return true;
}

bool ASlitherGameMode::TryRequestStartGame(
    AController* InController,
    const uint32 ExpectedLifeId)
{
    ASlitherPlayerState* PlayerState
        = GetSpawnPlayerState(InController);

    if (!PlayerState)
    {
        return false;
    }

    const UWorld* ThisWorld = GetWorld();
    if (!ThisWorld)
    {
        return false;
    }

    const ASlitherGameState* SlitherGameState
        = ThisWorld->GetGameState<ASlitherGameState>();
    if (!SlitherGameState)
    {
        return false;
    }

    if (const bool WorldReady
            = SlitherGameState->IsWorldReady();
        !WorldReady)
    {
        return false;
    }

    const APawn* ExistingPawn = InController->GetPawn();
    const int32 RequestIndex = FindSpawnRequest(InController);
    const double ServerTime = GetSpawnServerTime();

    if (ExistingPawn || RequestIndex != INDEX_NONE || ServerTime < 0.0)
    {
        return false;
    }

    bool Accepted = false;

    if (ExpectedLifeId == 0)
    {
        Accepted = PlayerState->TryQueueFirstSpawn();
    }
    else
    {
        Accepted = PlayerState->TryQueueRespawn(
            ExpectedLifeId, ServerTime);
    }

    if (!Accepted)
    {
        return false;
    }

    FSlitherSpawnRequest Request;
    Request.Controller = InController;
    Request.PlayerState = PlayerState;
    Request.ExpectedLifeId = ExpectedLifeId;
    Request.NextAttemptServerTime = ServerTime;
    SpawnRequests.Emplace(Request);
    return true;
}

void ASlitherGameMode::ProcessSpawnRequests(
    const double ServerTime)
{
    if (ProcessingSpawnRequests || EndingPlay)
    {
        return;
    }

    ProcessingSpawnRequests = true;
    const int32 RequestCount = SpawnRequests.Num();

    for (int32 Index = RequestCount - 1; Index >= 0; --Index)
    {
        const FSlitherSpawnRequest Request = SpawnRequests[Index];
        const bool Current = IsSpawnRequestCurrent(Request);

        if (!Current)
        {
            SpawnRequests.RemoveAt(Index, 1, EAllowShrinking::No);
        }
    }

    TArray<FSlitherSpawnRequest> OrderedRequests = SpawnRequests;
    OrderedRequests.Sort(CompareSpawnRequests);

    for (const FSlitherSpawnRequest& Request : OrderedRequests)
    {
        if (EndingPlay)
        {
            break;
        }

        const bool Current = IsSpawnRequestCurrent(Request);

        if (!Current)
        {
            continue;
        }

        const AController* Controller = Request.Controller.Get();
        const int32 Index = FindSpawnRequest(Controller);

        if (SpawnRequests[Index].Processing
            || SpawnRequests[Index].NextAttemptServerTime > ServerTime)
        {
            continue;
        }

        SpawnRequests[Index].Processing = true;

        if (const bool Spawned
                = TrySpawnRequest(Request);
            !Spawned)
        {
            if (const bool StillCurrent
                = IsSpawnRequestCurrent(Request))
            {
                const int32 RemainingIndex = FindSpawnRequest(Controller);
                SpawnRequests[RemainingIndex].Processing = false;
                SpawnRequests[RemainingIndex].NextAttemptServerTime =
                    ServerTime + FSlitherSettingsRespawn::SpawnRetryInterval;
            }
        }
    }

    ProcessingSpawnRequests = false;
}

void ASlitherGameMode::Logout(
    AController* Exiting)
{
    if (Exiting)
    {
        if (const int32 RequestIndex
                = FindSpawnRequest(Exiting);
            RequestIndex != INDEX_NONE)
        {
            SpawnRequests.RemoveAt(
                RequestIndex, 1, EAllowShrinking::No);
        }

        APawn* ExistingPawn = Exiting->GetPawn();

        if (const ASlitherWormPawn* WormPawn
            = Cast<ASlitherWormPawn>(ExistingPawn))
        {
            WormPawn->ResetInputState();
            WormSpawnComponent->UnregisterWormPawn(WormPawn);
        }
    }

    Super::Logout(Exiting);
}

void ASlitherGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndingPlay = true;
    SpawnRequests.Reset();
    Super::EndPlay(EndPlayReason);
}

APawn* ASlitherGameMode::SpawnDefaultPawnAtTransform_Implementation(
    AController* NewPlayer,
    const FTransform& SpawnTransform)
{
    if (const ASlitherPlayerState* PlayerState
            = GetSpawnPlayerState(NewPlayer);
        !PlayerState)
    {
        return nullptr;
    }

    if (const int32 RequestIndex
            = FindSpawnRequest(NewPlayer);
        RequestIndex == INDEX_NONE
        || !SpawnRequests[RequestIndex].Processing)
    {
        return nullptr;
    }

    if (const APawn* ExistingPawn
        = NewPlayer->GetPawn())
    {
        return nullptr;
    }

    UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
    ASlitherWormPawn* SpawnedPawn = WormSpawnComponent->SpawnWorm(
        NewPlayer, PawnClass, SpawnTransform);

    return SpawnedPawn;
}
