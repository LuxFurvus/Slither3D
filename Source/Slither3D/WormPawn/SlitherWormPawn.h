
#pragma once

#include "GameFramework/Pawn.h"
#include "Math/Color.h"

#include "SlitherWormMovementTypes.h"

#include "SlitherWormPawn.generated.h"

class ASlitherArena;
class USlitherBodyComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USlitherWormMovementComponent;
class USlitherWormReplicationComponent;
class USlitherWormVisualComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;


UCLASS()
class ASlitherWormPawn : public APawn
{
    GENERATED_BODY()

private:

    const FName WormColorParameterName = FName("WormColor");

    UPROPERTY(ReplicatedUsing = OnRep_LifeId)
    uint32 LifeId = 0;

    UPROPERTY(ReplicatedUsing = OnRep_WormColor)
    FLinearColor WormColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, Category = "Worm")
    TObjectPtr<UMaterialInterface> WormTailMaterial = nullptr;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> WormTailDynamicMaterial = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Worm")
    TObjectPtr<USphereComponent> HeadCollision = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Worm")
    TObjectPtr<USceneComponent> WormRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Worm")
    TObjectPtr<UStaticMeshComponent> HeadMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Worm")
    TObjectPtr<UInstancedStaticMeshComponent> BodyMesh = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherBodyComponent> WormBodyComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherWormMovementComponent> WormMovementComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherWormReplicationComponent> WormReplicationComponent = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherWormVisualComponent> WormVisualComponent = nullptr;

private:

    ASlitherWormPawn();

    void UpdateTailMaterial();

    UFUNCTION()
    void OnRep_LifeId() const;

    UFUNCTION()
    void OnRep_WormColor();

protected:

    void PostInitializeComponents() override;

    void BeginPlay() override;

public:

    FVector GetVisualHeadLocation() const;

    uint32 GetLifeId() const;

    bool HasActiveLife() const;

    USlitherBodyComponent* GetBodyComponent() const;

    const FSlitherWormInputState& GetInputState() const;

    bool InitializeLife(const uint32 NewLifeId);

    void ResetInputState() const;

    bool SubmitInput(
        const uint32 InputLifeId,
        const uint32 InputSequence,
        const FVector& AimDirection,
        const bool WantsBoost,
        const double InputTime) const;

    void SimulateMovement(
        const ASlitherArena* Arena,
        double StepSeconds,
        double ServerTime,
        FSlitherWormMovementResult& OutResult) const;

    void PublishSnapshot(
        const uint32 SimulationFrame,
        double ServerTime) const;

    void UpdateVisualState() const;

    bool TryBufferVisualSnapshot(
        const FSlitherWormSnapshot& Snapshot) const;

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
