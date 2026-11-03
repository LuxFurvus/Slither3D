
#include "SlitherWormCollisionComponent.h"

#include "Slither3D/Settings/SlitherGameSettingsTypes.h"


USlitherWormCollisionComponent::USlitherWormCollisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

bool USlitherWormCollisionComponent::HasServerAuthority() const
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return false;
    }

    const bool Authority = OwnerActor->HasAuthority();
    return Authority;
}

void USlitherWormCollisionComponent::GetCellBounds(
    const FSlitherCollisionElement& Element,
    FIntPoint& OutMinimum,
    FIntPoint& OutMaximum)
{
    const double MinimumX = FMath::Min(
        Element.PreviousLocation.X, Element.CurrentLocation.X);
    const double MinimumY = FMath::Min(
        Element.PreviousLocation.Y, Element.CurrentLocation.Y);
    const double MaximumX = FMath::Max(
        Element.PreviousLocation.X, Element.CurrentLocation.X);
    const double MaximumY = FMath::Max(
        Element.PreviousLocation.Y, Element.CurrentLocation.Y);

    constexpr double CellSize
        = FSlitherSettingsCollision::CellSize;

    OutMinimum.X = FMath::FloorToInt((MinimumX - Element.Radius) / CellSize);
    OutMinimum.Y = FMath::FloorToInt((MinimumY - Element.Radius) / CellSize);
    OutMaximum.X = FMath::FloorToInt((MaximumX + Element.Radius) / CellSize);
    OutMaximum.Y = FMath::FloorToInt((MaximumY + Element.Radius) / CellSize);
}

void USlitherWormCollisionComponent::AddElement(
    const FSlitherCollisionElement& Element)
{
    const int32 ElementIndex = Elements.Emplace(Element);
    FIntPoint Minimum;
    FIntPoint Maximum;
    GetCellBounds(Element, Minimum, Maximum);

    for (int32 X = Minimum.X; X <= Maximum.X; ++X)
    {
        for (int32 Y = Minimum.Y; Y <= Maximum.Y; ++Y)
        {
            const FIntPoint Cell(X, Y);
            TArray<int32>& Indices = Cells.FindOrAdd(Cell);
            Indices.Emplace(ElementIndex);
        }
    }
}

void USlitherWormCollisionComponent::BuildCollisionGrid(
    const TArray<FSlitherWormStepState>& StepStates,
    const TArray<FSlitherFoodCollisionEntry>& FoodEntries)
{
    Elements.Reset();
    Cells.Reset();
    HeadElementIndices.Reset();
    Contacts.Reset();
    CandidateIndices.Reset();

    if (const bool Authority
            = HasServerAuthority();
        !Authority)
    {
        return;
    }

    const int32 StepCount = StepStates.Num();
    HeadElementIndices.Init(INDEX_NONE, StepCount);

    for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
    {
        const FSlitherWormStepState& State = StepStates[StepIndex];
        FSlitherCollisionElement Head;
        Head.Type = ESlitherCollisionElementType::Head;
        Head.OwnerStepIndex = StepIndex;
        Head.PreviousLocation = State.PreviousHeadLocation;
        Head.CurrentLocation = State.CurrentHeadLocation;
        Head.Radius = FSlitherSettingsBody::HeadRadius;
        HeadElementIndices[StepIndex] = Elements.Num();
        AddElement(Head);

        const int32 PreviousCount = State.PreviousSegmentLocations.Num();
        const int32 CurrentCount = State.CurrentSegmentLocations.Num();
        check(PreviousCount == CurrentCount);
        const int32 SegmentCount = FMath::Min(PreviousCount, CurrentCount);

        for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
        {
            FSlitherCollisionElement Segment;
            Segment.Type = ESlitherCollisionElementType::Body;
            Segment.OwnerStepIndex = StepIndex;
            Segment.SegmentIndex = SegmentIndex;
            Segment.PreviousLocation = State.PreviousSegmentLocations[SegmentIndex];
            Segment.CurrentLocation = State.CurrentSegmentLocations[SegmentIndex];
            Segment.Radius = FSlitherSettingsBody::BodySegmentRadius;
            AddElement(Segment);
        }
    }

    const int32 FoodCount = FoodEntries.Num();

    for (int32 FoodIndex = 0; FoodIndex < FoodCount; ++FoodIndex)
    {
        const FSlitherFoodCollisionEntry& Entry = FoodEntries[FoodIndex];
        FSlitherCollisionElement Food;
        Food.Type = ESlitherCollisionElementType::Food;
        Food.FoodEntryIndex = FoodIndex;
        Food.PreviousLocation = Entry.Location;
        Food.CurrentLocation = Entry.Location;
        Food.Radius = Entry.Radius;
        AddElement(Food);
    }
}

bool USlitherWormCollisionComponent::TryFindSphereContact(
    const FVector& FirstStart,
    const FVector& FirstEnd,
    const FVector& SecondStart,
    const FVector& SecondEnd,
    const double CombinedRadius,
    const double MaximumAlpha,
    double& OutContactAlpha)
{
    OutContactAlpha = 0.0;
    const FVector2D RelativeStart(
        FirstStart.X - SecondStart.X, FirstStart.Y - SecondStart.Y);
    const FVector2D FirstMovement(
        FirstEnd.X - FirstStart.X, FirstEnd.Y - FirstStart.Y);
    const FVector2D SecondMovement(
        SecondEnd.X - SecondStart.X, SecondEnd.Y - SecondStart.Y);
    const FVector2D RelativeMovement = FirstMovement - SecondMovement;
    const double StartDistanceSquared = RelativeStart.SizeSquared();
    const double C = StartDistanceSquared - CombinedRadius * CombinedRadius;

    if (C <= 0.0)
    {
        return true;
    }

    const double A = RelativeMovement.SizeSquared();
    const double StartDotMovement = FVector2D::DotProduct(
        RelativeStart, RelativeMovement);
    const double B = 2.0 * StartDotMovement;

    if (A <= 0.0 || B >= 0.0)
    {
        return false;
    }

    const double BSquare = B * B;
    const double FourAC = 4.0 * A * C;
    double Discriminant = BSquare - FourAC;
    const double Tolerance = NumericalTolerance
        * (BSquare + FourAC + 1.0);

    if (Discriminant < -Tolerance)
    {
        return false;
    }

    Discriminant = FMath::Max(Discriminant, 0.0);
    const double Root = FMath::Sqrt(Discriminant);
    const double Alpha = (2.0 * C) / (-B + Root);

    if (Alpha < -NumericalTolerance || Alpha > MaximumAlpha + NumericalTolerance)
    {
        return false;
    }

    OutContactAlpha
        = FMath::Clamp(Alpha, 0.0, MaximumAlpha);

    return true;
}

void USlitherWormCollisionComponent::FindHeadContacts(
    const int32 StepIndex,
    const TArray<FSlitherWormStepState>& StepStates,
    const TArray<FSlitherFoodCollisionEntry>& FoodEntries,
    TArray<FSlitherFoodContact>& OutFoodContacts)
{
    const FSlitherWormStepState& State = StepStates[StepIndex];
    const int32 HeadIndex = HeadElementIndices[StepIndex];
    const FSlitherCollisionElement& Head = Elements[HeadIndex];
    const double HeadMaximumAlpha = State.MovementResult.WallContactAlpha;
    CandidateIndices.Reset();
    FIntPoint Minimum;
    FIntPoint Maximum;
    GetCellBounds(Head, Minimum, Maximum);

    for (int32 X = Minimum.X; X <= Maximum.X; ++X)
    {
        for (int32 Y = Minimum.Y; Y <= Maximum.Y; ++Y)
        {
            const FIntPoint Cell(X, Y);
            const TArray<int32>* Indices = Cells.Find(Cell);

            if (!Indices)
            {
                continue;
            }

            for (const int32 ElementIndex : *Indices)
            {
                CandidateIndices.Emplace(ElementIndex);
            }
        }
    }

    for (const int32 ElementIndex : CandidateIndices)
    {
        const FSlitherCollisionElement& Element = Elements[ElementIndex];

        if (Element.OwnerStepIndex == StepIndex)
        {
            continue;
        }

        FVector OtherEnd = Element.CurrentLocation;
        double MaximumAlpha = HeadMaximumAlpha;

        if (Element.Type == ESlitherCollisionElementType::Head)
        {
            if (Element.OwnerStepIndex < StepIndex)
            {
                continue;
            }

            const FSlitherWormStepState& OtherState =
                StepStates[Element.OwnerStepIndex];
            OtherEnd = OtherState.MovementResult.IntendedHeadLocation;
            MaximumAlpha = FMath::Min(
                MaximumAlpha, OtherState.MovementResult.WallContactAlpha);
        }

        const double CombinedRadius = Head.Radius + Element.Radius;
        double ContactAlpha = 0.0;
        const bool FoundContact = TryFindSphereContact(
            State.PreviousHeadLocation, State.MovementResult.IntendedHeadLocation,
            Element.PreviousLocation, OtherEnd, CombinedRadius,
            MaximumAlpha, ContactAlpha);

        if (!FoundContact)
        {
            continue;
        }

        if (Element.Type == ESlitherCollisionElementType::Food)
        {
            const FSlitherFoodCollisionEntry& Food = FoodEntries[Element.FoodEntryIndex];
            FSlitherFoodContact Contact;
            Contact.Food = Food.Food;
            Contact.WormPawn = State.WormPawn;
            Contact.PlayerState = State.PlayerState;
            Contact.LifeId = State.LifeId;
            Contact.PlayerId = State.PlayerId;
            Contact.FoodId = Food.FoodId;
            Contact.ContactAlpha = ContactAlpha;

            OutFoodContacts.Emplace(Contact);
            continue;
        }

        const FSlitherWormStepState& OtherState = StepStates[Element.OwnerStepIndex];
        FSlitherWormContact Contact;
        Contact.VictimStepIndex = StepIndex;
        Contact.OtherStepIndex = Element.OwnerStepIndex;
        Contact.VictimPlayerId = State.PlayerId;
        Contact.OtherPlayerId = OtherState.PlayerId;
        Contact.SegmentIndex = Element.SegmentIndex;
        Contact.ContactAlpha = ContactAlpha;

        Contact.Reason
            = Element.Type == ESlitherCollisionElementType::Head
            ? ESlitherDeathReason::Head : ESlitherDeathReason::Body;

        Contacts.Emplace(Contact);
    }

    if (State.MovementResult.HitWall)
    {
        FSlitherWormContact Wall;
        Wall.VictimStepIndex = StepIndex;
        Wall.VictimPlayerId = State.PlayerId;
        Wall.ContactAlpha = HeadMaximumAlpha;
        Wall.Reason = ESlitherDeathReason::Wall;

        Contacts.Emplace(Wall);
    }
}

bool USlitherWormCollisionComponent::CompareContacts(
    const FSlitherWormContact& First,
    const FSlitherWormContact& Second)
{
    if (First.ContactAlpha != Second.ContactAlpha)
    {
        return First.ContactAlpha < Second.ContactAlpha;
    }

    if (First.Reason != Second.Reason)
    {
        return First.Reason < Second.Reason;
    }

    if (First.VictimPlayerId != Second.VictimPlayerId)
    {
        return First.VictimPlayerId < Second.VictimPlayerId;
    }

    if (First.OtherPlayerId != Second.OtherPlayerId)
    {
        return First.OtherPlayerId < Second.OtherPlayerId;
    }

    return First.SegmentIndex < Second.SegmentIndex;
}

bool USlitherWormCollisionComponent::CompareDeaths(
    const FSlitherWormDeathRecord& First,
    const FSlitherWormDeathRecord& Second)
{
    if (First.PlayerId != Second.PlayerId)
    {
        return First.PlayerId < Second.PlayerId;
    }

    return First.LifeId < Second.LifeId;
}

void USlitherWormCollisionComponent::AddDeathRecord(
    const int32 StepIndex,
    const FSlitherWormContact& Contact,
    const TArray<FSlitherWormStepState>& StepStates,
    TArray<FSlitherWormDeathRecord>& OutDeaths)
{
    const FSlitherWormStepState& Victim = StepStates[StepIndex];
    FSlitherWormDeathRecord Death;
    Death.WormPawn = Victim.WormPawn;
    Death.PlayerState = Victim.PlayerState;
    Death.LifeId = Victim.LifeId;
    Death.PlayerId = Victim.PlayerId;
    Death.Mass = Victim.Mass;
    Death.ContactAlpha = Contact.ContactAlpha;
    Death.Reason = Contact.Reason;
    Death.HeadLocation = Victim.CurrentHeadLocation;
    Death.SegmentLocations = Victim.CurrentSegmentLocations;

    if (Contact.Reason == ESlitherDeathReason::Body)
    {
        const FSlitherWormStepState& Killer = StepStates[Contact.OtherStepIndex];
        Death.KillerPlayerState = Killer.PlayerState;
        Death.KillerLifeId = Killer.LifeId;
        Death.KillerPlayerId = Killer.PlayerId;
    }

    OutDeaths.Emplace(Death);
}

void USlitherWormCollisionComponent::ResolveDeaths(
    const TArray<FSlitherWormStepState>& StepStates,
    TArray<FSlitherWormDeathRecord>& OutDeaths)
{
    Contacts.Sort(&USlitherWormCollisionComponent::CompareContacts);
    const int32 StepCount = StepStates.Num();
    const int32 ContactCount = Contacts.Num();
    DeathContactIndices.Init(INDEX_NONE, StepCount);
    int32 GroupStart = 0;

    while (GroupStart < ContactCount)
    {
        const double GroupTime = Contacts[GroupStart].ContactAlpha;
        int32 GroupEnd = GroupStart + 1;

        while (GroupEnd < ContactCount
            && Contacts[GroupEnd].ContactAlpha - GroupTime
                <= FSlitherSettingsCollision::ContactTimeTolerance)
        {
            ++GroupEnd;
        }

        EarlierDeathContactIndices = DeathContactIndices;

        for (int32 Index = GroupStart; Index < GroupEnd; ++Index)
        {
            const FSlitherWormContact& Contact = Contacts[Index];

            if (Contact.Reason != ESlitherDeathReason::Head
                || EarlierDeathContactIndices[Contact.VictimStepIndex] != INDEX_NONE
                || EarlierDeathContactIndices[Contact.OtherStepIndex] != INDEX_NONE)
            {
                continue;
            }

            if (DeathContactIndices[Contact.VictimStepIndex] == INDEX_NONE)
            {
                DeathContactIndices[Contact.VictimStepIndex] = Index;
            }

            if (DeathContactIndices[Contact.OtherStepIndex] == INDEX_NONE)
            {
                DeathContactIndices[Contact.OtherStepIndex] = Index;
            }
        }

        for (int32 Index = GroupStart; Index < GroupEnd; ++Index)
        {
            const FSlitherWormContact& Contact = Contacts[Index];

            if (Contact.Reason == ESlitherDeathReason::Wall
                && DeathContactIndices[Contact.VictimStepIndex] == INDEX_NONE)
            {
                DeathContactIndices[Contact.VictimStepIndex] = Index;
            }
        }

        for (int32 Index = GroupStart; Index < GroupEnd; ++Index)
        {
            const FSlitherWormContact& Contact = Contacts[Index];

            if (Contact.Reason != ESlitherDeathReason::Body
                || EarlierDeathContactIndices[Contact.VictimStepIndex] != INDEX_NONE)
            {
                continue;
            }

            const int32 ExistingIndex
                = DeathContactIndices[Contact.VictimStepIndex];

            if (ExistingIndex == INDEX_NONE)
            {
                DeathContactIndices[Contact.VictimStepIndex] = Index;
                continue;
            }

            const FSlitherWormContact& Existing = Contacts[ExistingIndex];

            if (Existing.Reason == ESlitherDeathReason::Body
                && (Contact.OtherPlayerId < Existing.OtherPlayerId
                    || (Contact.OtherPlayerId == Existing.OtherPlayerId
                        && Contact.SegmentIndex < Existing.SegmentIndex)))
            {
                DeathContactIndices[Contact.VictimStepIndex] = Index;
            }
        }

        GroupStart = GroupEnd;
    }

    for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
    {
        const int32 ContactIndex = DeathContactIndices[StepIndex];

        if (ContactIndex != INDEX_NONE)
        {
            const FSlitherWormContact& Contact = Contacts[ContactIndex];
            AddDeathRecord(StepIndex, Contact, StepStates, OutDeaths);
        }
    }
}

void USlitherWormCollisionComponent::FindContacts(
    const TArray<FSlitherWormStepState>& StepStates,
    const TArray<FSlitherFoodCollisionEntry>& FoodEntries,
    TArray<FSlitherFoodContact>& OutFoodContacts,
    TArray<FSlitherWormDeathRecord>& OutDeaths)
{
    Contacts.Reset();
    OutFoodContacts.Reset();
    OutDeaths.Reset();
    const bool Authority = HasServerAuthority();

    if (!Authority)
    {
        return;
    }

    const int32 StepCount = StepStates.Num();
    const int32 HeadCount = HeadElementIndices.Num();
    check(StepCount == HeadCount);

    if (StepCount != HeadCount)
    {
        return;
    }

    for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
    {
        FindHeadContacts(StepIndex, StepStates, FoodEntries, OutFoodContacts);
    }

    ResolveDeaths(StepStates, OutDeaths);
    OutDeaths.Sort(&USlitherWormCollisionComponent::CompareDeaths);
}