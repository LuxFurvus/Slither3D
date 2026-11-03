
#include "SlitherFoodManagerComponent.h"
#include "SlitherWormSimulationTypes.h"

#include "Engine/World.h"
#include "Misc/AssertionMacros.h"

#include "SlitherFood.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"
#include "Slither3D/Slither3D.h"
#include "Slither3D/SlitherArena.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"
#include "Slither3D/WormPawn/SlitherBodyComponent.h"
#include "Slither3D/WormPawn/SlitherWormPawn.h"


USlitherFoodManagerComponent::USlitherFoodManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

bool USlitherFoodManagerComponent::HasServerAuthority() const
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return false;
    }

    const bool Authority
        = OwnerActor->HasAuthority();

    return Authority;
}

UClass* USlitherFoodManagerComponent::GetRequiredFoodClass() const
{
    UClass* AssignedFoodClass
        = FoodClass.Get();

    checkf(AssignedFoodClass,
        TEXT("Assign BP_SlitherFood to FoodClass in BP_SlitherGameMode."));

    return AssignedFoodClass;
}

void USlitherFoodManagerComponent::RegisterFood(
    ASlitherFood* Food,
    const bool NaturalFood)
{
    if (!Food || EndingPlay)
    {
        return;
    }

    const bool Authority = HasServerAuthority();
    const bool Available = Food->IsAvailable();

    if (!Authority || !Available)
    {
        return;
    }

    const uint64 FoodId = Food->GetFoodId();
    Foods.Emplace(FoodId, Food);

    if (NaturalFood)
    {
        NaturalFoodIds.Emplace(FoodId);
    }
    else
    {
        DeathFoodIdsInCreationOrder.Emplace(FoodId);
        Food->SetLifeSpan(FSlitherSettingsFood::DeathFoodLifetime);
        EnforceDeathFoodLimit();
    }
}

void USlitherFoodManagerComponent::UnregisterFood(
    const uint64 FoodId)
{
    const bool Authority = HasServerAuthority();

    if (!Authority || EndingPlay)
    {
        return;
    }

    Foods.Remove(FoodId);
    NaturalFoodIds.Remove(FoodId);
    DeathFoodIdsInCreationOrder.Remove(FoodId);
}

void USlitherFoodManagerComponent::RemoveUnavailableFood()
{
    for (auto FoodIterator = Foods.CreateIterator(); FoodIterator; ++FoodIterator)
    {
        const TWeakObjectPtr<ASlitherFood>& FoodReference
            = FoodIterator.Value();

        const ASlitherFood* Food = FoodReference.Get();
        bool Available = false;

        if (Food)
        {
            Available = Food->IsAvailable();
        }

        if (!Available)
        {
            const uint64 FoodId = FoodIterator.Key();
            NaturalFoodIds.Remove(FoodId);
            DeathFoodIdsInCreationOrder.Remove(FoodId);
            FoodIterator.RemoveCurrent();
        }
    }
}

int32 USlitherFoodManagerComponent::ChooseNaturalNutrition()
{
    const float Roll = FMath::FRand();
    float CumulativeProbability = 0.0f;

    for (const FSlitherFoodType& FoodType : FSlitherSettingsFood::FoodTypes)
    {
        CumulativeProbability += FoodType.SpawnProbability;

        if (Roll < CumulativeProbability)
        {
            return FoodType.Nutrition;
        }
    }

    constexpr int32 LastIndex
        = UE_ARRAY_COUNT(FSlitherSettingsFood::FoodTypes) - 1;

    return FSlitherSettingsFood::FoodTypes[LastIndex].Nutrition;
}

bool USlitherFoodManagerComponent::CanPlaceFood(
    const ASlitherArena* Arena,
    const FVector2D& Center,
    const double Radius,
    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns)
{
    const bool InsideArena
        = Arena->CanPlaceCircle(Center, Radius);

    if (!InsideArena)
    {
        return false;
    }

    for (const TWeakObjectPtr<ASlitherWormPawn>& PawnReference : WormPawns)
    {
        const ASlitherWormPawn* WormPawn = PawnReference.Get();

        if (!WormPawn)
        {
            continue;
        }

        const bool BeingDestroyed = WormPawn->IsActorBeingDestroyed();
        const uint32 LifeId = WormPawn->GetLifeId();
        const bool LifeActive = WormPawn->HasActiveLife();

        if (BeingDestroyed || !LifeActive || LifeId == 0)
        {
            continue;
        }

        const FVector HeadLocation = WormPawn->GetActorLocation();
        const FVector2D HeadCenter(HeadLocation.X, HeadLocation.Y);
        const FVector2D HeadDelta = Center - HeadCenter;
        const double HeadDistanceSquared = HeadDelta.SizeSquared();
        const double HeadSeparation = Radius + FSlitherSettingsBody::HeadRadius;

        if (HeadDistanceSquared <= HeadSeparation * HeadSeparation)
        {
            return false;
        }

        const USlitherBodyComponent* BodyComponent = WormPawn->GetBodyComponent();

        if (!BodyComponent)
        {
            continue;
        }

        const TArray<FVector>& Segments = BodyComponent->GetServerSegmentLocations();
        const double BodySeparation = Radius + FSlitherSettingsBody::BodySegmentRadius;

        for (const FVector& Segment : Segments)
        {
            const FVector2D SegmentCenter(Segment.X, Segment.Y);
            const FVector2D SegmentDelta = Center - SegmentCenter;
            const double SegmentDistanceSquared = SegmentDelta.SizeSquared();

            if (SegmentDistanceSquared <= BodySeparation * BodySeparation)
            {
                return false;
            }
        }
    }

    return true;
}

void USlitherFoodManagerComponent::EnforceDeathFoodLimit()
{
    int32 DeathCount = DeathFoodIdsInCreationOrder.Num();

    while (DeathCount > FSlitherSettingsFood::MaxDeathFoodCount)
    {
        const uint64 OldestId = DeathFoodIdsInCreationOrder[0];
        const TWeakObjectPtr<ASlitherFood>* FoodReference = Foods.Find(OldestId);
        ASlitherFood* Food = nullptr;

        if (FoodReference)
        {
            Food = FoodReference->Get();
        }

        UnregisterFood(OldestId);

        if (Food)
        {
            Food->Destroy();
        }

        DeathCount = DeathFoodIdsInCreationOrder.Num();
    }
}

bool USlitherFoodManagerComponent::CompareContacts(
    const FSlitherFoodContact& First,
    const FSlitherFoodContact& Second)
{
    if (First.ContactAlpha != Second.ContactAlpha)
    {
        return First.ContactAlpha < Second.ContactAlpha;
    }

    if (First.PlayerId != Second.PlayerId)
    {
        return First.PlayerId < Second.PlayerId;
    }

    return First.FoodId < Second.FoodId;
}

void USlitherFoodManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    (void) GetRequiredFoodClass();
}

void USlitherFoodManagerComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    EndingPlay = true;
    Foods.Reset();
    NaturalFoodIds.Reset();
    DeathFoodIdsInCreationOrder.Reset();
    PendingDeathFood.Reset();
    Super::EndPlay(EndPlayReason);
}

ASlitherFood* USlitherFoodManagerComponent::SpawnFood(
    const int32 Nutrition,
    const FVector& Location,
    const bool NaturalFood)
{
    const bool Authority = HasServerAuthority();

    if (!Authority || EndingPlay || Nutrition <= 0)
    {
        return nullptr;
    }

    UWorld* ThisWorld = GetWorld();

    if (!ThisWorld)
    {
        return nullptr;
    }

    const FTransform SpawnTransform(
        FRotator::ZeroRotator, Location, FVector::OneVector);

    UClass* AssignedFoodClass
        = GetRequiredFoodClass();

    ASlitherFood* Food = ThisWorld->SpawnActorDeferred<ASlitherFood>(
        AssignedFoodClass, SpawnTransform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Food)
    {
        return nullptr;
    }

    const uint64 FoodId = NextFoodId;
    ++NextFoodId;

    if (NextFoodId == 0)
    {
        ++NextFoodId;
    }

    Food->InitializeFood(Nutrition, FoodId, this);
    Food->FinishSpawning(SpawnTransform);

    const bool Available = Food->IsAvailable();

    if (!Available)
    {
        Food->Destroy();
        return nullptr;
    }

    RegisterFood(Food, NaturalFood);
    return Food;
}

TArray<FSlitherFoodCollisionEntry> USlitherFoodManagerComponent::GatherCollisionEntries()
{
    TArray<FSlitherFoodCollisionEntry> Entries;

    const bool Authority
        = HasServerAuthority();

    if (!Authority || EndingPlay)
    {
        return Entries;
    }

    RemoveUnavailableFood();

    for (const auto& FoodEntry : Foods)
    {
        ASlitherFood* Food = FoodEntry.Value.Get();

        if (!Food)
        {
            continue;
        }

        const int32 Nutrition = Food->GetNutrition();
        FLinearColor Color;
        FSlitherFoodCollisionEntry Entry;
        Entry.Food = Food;
        Entry.FoodId = FoodEntry.Key;
        Entry.Location = Food->GetActorLocation();

        ASlitherFood::CalculateAppearance(
            Nutrition, Entry.Radius, Color);

        Entries.Emplace(Entry);
    }

    return Entries;
}

void USlitherFoodManagerComponent::CollectFood(
    TArray<FSlitherFoodContact>& Contacts)
{
    const bool Authority = HasServerAuthority();

    if (!Authority || EndingPlay)
    {
        return;
    }

    RemoveUnavailableFood();
    Contacts.Sort(&USlitherFoodManagerComponent::CompareContacts);

    for (const FSlitherFoodContact& Contact : Contacts)
    {
        ASlitherFood* Food = Contact.Food.Get();
        const ASlitherWormPawn* WormPawn = Contact.WormPawn.Get();
        ASlitherPlayerState* PlayerState = Contact.PlayerState.Get();

        if (!Food || !WormPawn || !PlayerState)
        {
            continue;
        }

        const bool PawnBeingDestroyed = WormPawn->IsActorBeingDestroyed();
        const uint32 LifeId = WormPawn->GetLifeId();
        const AController* Controller = WormPawn->GetController();
        const ASlitherPlayerState* CurrentPlayerState =
            WormPawn->GetPlayerState<ASlitherPlayerState>();
        const uint32 PlayerLifeId = PlayerState->GetLifeId();
        const bool PlayerBeingDestroyed = PlayerState->IsActorBeingDestroyed();
        const bool Alive = PlayerState->IsAlive();
        const uint64 FoodId = Food->GetFoodId();

        if (PawnBeingDestroyed
            || PlayerBeingDestroyed
            || !Controller
            || !Alive
            || FoodId != Contact.FoodId
            || LifeId != Contact.LifeId
            || PlayerLifeId != Contact.LifeId
            || CurrentPlayerState != PlayerState)
        {
            continue;
        }

        int32 AcceptedMass = 0;
        Food->TryConsumeBy(PlayerState, AcceptedMass);
    }
}

void USlitherFoodManagerComponent::QueueDeathFood(
    const ASlitherArena* Arena,
    const FSlitherWormDeathRecord& Death)
{
    const bool Authority
        = HasServerAuthority();

    if (!Authority || EndingPlay || !Arena)
    {
        return;
    }

    const double DeathMass = Death.Mass;
 
    const double RequestedDropMass
        = DeathMass
        * FSlitherSettingsFood::DeathFoodMassFraction;
 
    const int32 DropMass
        = FMath::FloorToInt(RequestedDropMass);

    if (DropMass <= 0)
    {
        return;
    }

    const int32 SegmentCount = Death.SegmentLocations.Num();
    const int32 PlacementCount = FMath::Max(SegmentCount, 1);
    constexpr int32 PreferredNutrition =
        FSlitherSettingsFood::PreferredDeathFoodNutrition;
    const int32 DesiredCount = (DropMass - 1) / PreferredNutrition + 1;
    const int32 FoodCount = FMath::Min(PlacementCount, DesiredCount);
    const int32 BaseNutrition = DropMass / FoodCount;
    const int32 Remainder = DropMass % FoodCount;
    const FBox2D Bounds = Arena->GetPlayableBounds();
    const FVector ArenaLocation = Arena->GetActorLocation();

    if (SegmentCount == 0)
    {
        UE_LOG(SlitherLog,
            Warning, TEXT("Death food uses head position: Player=%d Life=%u has no body segments."),
            Death.PlayerId, Death.LifeId);
    }

    for (int32 FoodIndex = 0; FoodIndex < FoodCount; ++FoodIndex)
    {
        FSlitherPendingDeathFood Pending;
        Pending.Nutrition = BaseNutrition + (FoodIndex < Remainder ? 1 : 0);
        Pending.Location = Death.HeadLocation;

        if (SegmentCount > 0)
        {
            const int32 SegmentIndex =
                ((2 * FoodIndex + 1) * SegmentCount) / (2 * FoodCount);
            Pending.Location = Death.SegmentLocations[SegmentIndex];
        }

        double Radius = 0.0;
        FLinearColor Color;
        ASlitherFood::CalculateAppearance(Pending.Nutrition, Radius, Color);

        Pending.Location.X = FMath::Clamp(
            Pending.Location.X, Bounds.Min.X + Radius, Bounds.Max.X - Radius);

        Pending.Location.Y = FMath::Clamp(
            Pending.Location.Y, Bounds.Min.Y + Radius, Bounds.Max.Y - Radius);

        Pending.Location.Z = ArenaLocation.Z + Radius;

        PendingDeathFood.Emplace(Pending);
    }
}

void USlitherFoodManagerComponent::SpawnPendingDeathFood()
{
    const bool Authority = HasServerAuthority();

    if (!Authority || EndingPlay)
    {
        return;
    }

    const int32 PendingCount
        = PendingDeathFood.Num();
    int32 FailedCount = 0;

    for (int32 Index = 0; Index < PendingCount; ++Index)
    {
        const FSlitherPendingDeathFood Pending
            = PendingDeathFood[Index];

        const ASlitherFood* Food
            = SpawnFood(Pending.Nutrition, Pending.Location, false);

        if (!Food)
        {
            PendingDeathFood[FailedCount] = Pending;
            ++FailedCount;
        }
    }

    PendingDeathFood.SetNum(FailedCount, EAllowShrinking::No);

    if (FailedCount > 0)
    {
        if (const UWorld* ThisWorld = GetWorld())
        {
            const double ServerNow
                = ThisWorld->GetTimeSeconds();

            if (LastDeathFoodWarningTime < 0.0
                || ServerNow - LastDeathFoodWarningTime >= 1.0)
            {
                UE_LOG(SlitherLog, Warning,
                    TEXT("Death food spawn failed; %d portions will retry next substep."),
                    FailedCount);

                LastDeathFoodWarningTime = ServerNow;
            }
        }
    }
}

void USlitherFoodManagerComponent::ReplenishNaturalFood(
    const ASlitherArena* Arena,
    const TArray<TWeakObjectPtr<ASlitherWormPawn>>& WormPawns)
{
    const bool Authority
        = HasServerAuthority();

    if (!Authority || EndingPlay || !Arena)
    {
        return;
    }

    RemoveUnavailableFood();

    const int32 NaturalCount = NaturalFoodIds.Num();

    const int32 MissingCount
        = FSlitherSettingsFood::NaturalFoodCount - NaturalCount;

    const int32 SpawnBudget = FMath::Min(
        MissingCount,
        FSlitherSettingsFood::MaxNaturalFoodSpawnsPerSubstep);

    const FBox2D Bounds = Arena->GetPlayableBounds();

    const FVector ArenaLocation = Arena->GetActorLocation();

    for (int32 SpawnIndex = 0; SpawnIndex < SpawnBudget; ++SpawnIndex)
    {
        const int32 Nutrition = ChooseNaturalNutrition();
        double Radius = 0.0;
        FLinearColor Color;
        ASlitherFood::CalculateAppearance(Nutrition, Radius, Color);
        const double MinimumX = Bounds.Min.X + Radius;
        const double MaximumX = Bounds.Max.X - Radius;
        const double MinimumY = Bounds.Min.Y + Radius;
        const double MaximumY = Bounds.Max.Y - Radius;

        if (MinimumX > MaximumX || MinimumY > MaximumY)
        {
            continue;
        }

        for (int32 Attempt = 0;
            Attempt < FSlitherSettingsFood::MaxPlacementAttemptsPerFood; ++Attempt)
        {
            const double X = FMath::FRandRange(MinimumX, MaximumX);
            const double Y = FMath::FRandRange(MinimumY, MaximumY);

            const FVector2D Center(X, Y);

            const bool CanPlace
                = CanPlaceFood(Arena, Center, Radius, WormPawns);

            if (!CanPlace)
            {
                continue;
            }

            const FVector Location(X, Y, ArenaLocation.Z + Radius);
            SpawnFood(Nutrition, Location, true);
            break;
        }
    }
}

int32 USlitherFoodManagerComponent::GetNaturalFoodCount() const
{
    const int32 Count = NaturalFoodIds.Num();
    return Count;
}

int32 USlitherFoodManagerComponent::GetDeathFoodCount() const
{
    const int32 Count = DeathFoodIdsInCreationOrder.Num();
    return Count;
}

int32 USlitherFoodManagerComponent::GetTotalFoodCount() const
{
    const int32 Count = Foods.Num();
    return Count;
}
