
#include "SlitherFood.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AssertionMacros.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include "Slither3D/GameMode/SlitherFoodManagerComponent.h"
#include "Slither3D/Settings/SlitherGameSettingsTypes.h"
#include "Slither3D/PlayerState/SlitherPlayerState.h"


ASlitherFood::ASlitherFood()
{
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = true;
    AActor::SetReplicateMovement(false);

    FoodRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FoodRoot"));
    SetRootComponent(FoodRoot);

    FoodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FoodMesh"));
    FoodMesh->SetupAttachment(FoodRoot);
    FoodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FoodMesh->SetGenerateOverlapEvents(false);
    FoodMesh->SetSimulatePhysics(false);
    FoodMesh->SetEnableGravity(false);
    FoodMesh->SetCanEverAffectNavigation(false);
    FoodMesh->SetIsReplicated(false);
    FoodMesh->SetVisibility(false);

    const ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* SphereMesh
        = SphereMeshFinder.Object;

    checkf(SphereMesh,
        TEXT("Required Unreal Engine sphere mesh is unavailable."));

    FoodMesh->SetStaticMesh(SphereMesh);
}

void ASlitherFood::UpdateAppearance()
{
    UMaterialInterface* Material
        = FoodMaterial.Get();

    checkf(Material,
        TEXT("Assign M_SlitherFood to FoodMaterial in BP_SlitherFood Class Defaults."));

    if (const ENetMode NetMode = GetNetMode();
        NetMode == NM_DedicatedServer)
    {
        return;
    }

    if (Nutrition <= 0)
    {
        FoodMesh->SetVisibility(false);
        return;
    }

    double Radius = 0.0;
    FLinearColor Color;
    CalculateAppearance(Nutrition, Radius, Color);

    const double ScaleFactor
        = Radius / FSlitherSettingsBody::UnitSphereRadius;

    const FVector MeshScale(ScaleFactor);
    FoodMesh->SetRelativeScale3D(MeshScale);

    if (!DynamicMaterial)
    {
        DynamicMaterial
            = FoodMesh->CreateDynamicMaterialInstance(0, Material);

        checkf(DynamicMaterial,
            TEXT("Cannot create food MID. Assign a valid material to FoodMaterial in BP_SlitherFood."));
    }

    DynamicMaterial->SetVectorParameterValue(
        ColorParameterName, Color);

    FoodMesh->SetVisibility(true);
}

void ASlitherFood::OnRep_Nutrition()
{
    UpdateAppearance();
}

void ASlitherFood::BeginPlay()
{
    Super::BeginPlay();
    UpdateAppearance();
}

void ASlitherFood::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (USlitherFoodManagerComponent* FoodManagerPtr
        = FoodManager.Get())
    {
        FoodManagerPtr->UnregisterFood(FoodId);
    }

    FoodManager.Reset();
    Super::EndPlay(EndPlayReason);
}

void ASlitherFood::InitializeFood(
    const int32 InNutrition,
    const uint64 InFoodId,
    USlitherFoodManagerComponent* InManager)
{
    const bool HasServerAuthority
        = HasAuthority();

    const bool AreArgsValid
        = InNutrition > 0
        || InFoodId != 0
        || InManager;

    const bool IsAlreadyInitialized
        = Nutrition > 0;

    if (!HasServerAuthority
        || IsAlreadyInitialized
        || !AreArgsValid)
    {
        return;
    }

    Nutrition = InNutrition;
    FoodId = InFoodId;
    FoodManager = InManager;

    UpdateAppearance();
}

int32 ASlitherFood::GetNutrition() const
{
    return Nutrition;
}

uint64 ASlitherFood::GetFoodId() const
{
    return FoodId;
}

bool ASlitherFood::IsAvailable() const
{
    const bool BeingDestroyed = IsActorBeingDestroyed();
    return Nutrition > 0 && FoodId > 0 && !Consumed && !BeingDestroyed;
}

bool ASlitherFood::TryConsumeBy(
    ASlitherPlayerState* PlayerState,
    int32& OutAcceptedMass)
{
    OutAcceptedMass = 0;

    const bool HasServerAuthority = HasAuthority();
    const bool Available = IsAvailable();

    if (!HasServerAuthority || !Available || !PlayerState)
    {
        return false;
    }

    const bool PlayerStateBeingDestroyed
        = PlayerState->IsActorBeingDestroyed();
    const bool PlayerStateHasAuthority
        = PlayerState->HasAuthority();
    const bool PlayerAlive = PlayerState->IsAlive();

    if (PlayerStateBeingDestroyed || !PlayerStateHasAuthority || !PlayerAlive)
    {
        return false;
    }

    Consumed = true;
    OutAcceptedMass = PlayerState->AddMass(Nutrition);

    if (USlitherFoodManagerComponent* FoodManagerPtr
        = FoodManager.Get())
    {
        FoodManagerPtr->UnregisterFood(FoodId);
    }

    Destroy();
    return true;
}

void ASlitherFood::CalculateAppearance(
    const int32 InNutrition,
    double& OutRadius,
    FLinearColor& OutColor)
{
    constexpr int32 TypeCount
        = UE_ARRAY_COUNT(FSlitherSettingsFood::FoodTypes);

    const FSlitherFoodType& FirstType
        = FSlitherSettingsFood::FoodTypes[0];

    OutRadius = FirstType.Radius;
    OutColor = FirstType.FoodColor;

    if (InNutrition <= FirstType.Nutrition)
    {
        return;
    }

    for (int32 TypeIndex = 1; TypeIndex < TypeCount; ++TypeIndex)
    {
        const FSlitherFoodType& LowerType =
            FSlitherSettingsFood::FoodTypes[TypeIndex - 1];
        const FSlitherFoodType& UpperType =
            FSlitherSettingsFood::FoodTypes[TypeIndex];

        if (InNutrition <= UpperType.Nutrition)
        {
            const double NutritionRange = UpperType.Nutrition - LowerType.Nutrition;
            const double Alpha = (InNutrition - LowerType.Nutrition) / NutritionRange;
            const double LowerRadius = LowerType.Radius;
            const double UpperRadius = UpperType.Radius;

            const float ColorAlpha = static_cast<float>(Alpha);

            OutRadius
                = FMath::Lerp(LowerRadius, UpperRadius, Alpha);
            OutColor
                = FMath::Lerp(LowerType.FoodColor, UpperType.FoodColor, ColorAlpha);

            return;
        }
    }

    const FSlitherFoodType& LastType =
        FSlitherSettingsFood::FoodTypes[TypeCount - 1];

    OutRadius = LastType.Radius;
    OutColor = LastType.FoodColor;
}

void ASlitherFood::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASlitherFood, Nutrition);
}
