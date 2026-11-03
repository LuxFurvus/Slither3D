
#pragma once

#include "Components/ActorComponent.h"

#include "SlitherWormMovementTypes.h"

#include "SlitherWormVisualComponent.generated.h"

class USlitherBodyComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMeshComponent;


UCLASS()
class USlitherWormVisualComponent : public UActorComponent
{
    GENERATED_BODY()

private:

    static constexpr uint32 MaxForwardSequenceDelta = MAX_uint32 / 2u;

    TArray<FSlitherWormSnapshot> SnapshotBuffer;
    TArray<FTransform> PendingBodyTransforms;
    FSlitherWormSnapshot LastAcceptedSnapshot;

    bool HasAcceptedSnapshot = false;
    bool HasRenderedState = false;
    double LastRenderedHeadDistance = 0.0;
    double LastRenderedBodyLength = 0.0;

    uint32 BufferedLifeId = 0;
    uint64 LastVisualUpdateFrame = MAX_uint64;
    double LastRenderTime = -1.0;

    UPROPERTY()
    TObjectPtr<USceneComponent> WormRoot = nullptr;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> HeadMesh = nullptr;

    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> BodyMesh = nullptr;

    UPROPERTY()
    TObjectPtr<USlitherBodyComponent> WormBodyComponent = nullptr;

private:

    USlitherWormVisualComponent();

    void DiscardObsoleteSnapshots();

    void TrimBodyHistory() const;

    bool ApplyVisualState(
        const FVector& HeadLocation,
        const FVector& HeadDirection) const;

    void ApplyVisualTransform(
        const FVector& HeadLocation,
        const FVector& HeadDirection) const;

protected:

    void BeginPlay() override;

    void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

public:

    void TickComponent(
        float DeltaSeconds,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    void InitializeVisualComponents(
        USceneComponent* InWormRoot,
        UStaticMeshComponent* InHeadMesh,
        UInstancedStaticMeshComponent* InBodyMesh,
        USlitherBodyComponent* InWormBodyComponent);

    void InitializeVisualState() const;

    FVector GetVisualHeadLocation() const;

    void ResetForLife(
        const uint32 LifeId);

    bool TryBufferSnapshot(
        const FSlitherWormSnapshot& Snapshot);

    void UpdateVisualState();
};
