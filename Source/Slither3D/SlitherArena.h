
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SlitherArena.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;


UCLASS()
class ASlitherArena : public AActor
{
    GENERATED_BODY()
private:

    static constexpr double UnitCubeSize = 100.0;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<USceneComponent> ArenaRoot;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> Floor;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> NegativeXWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> PositiveXWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> NegativeYWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> PositiveYWall;

private:

    ASlitherArena();

    void InitComponents();

    void ImplementStaticMeshes() const;

    static void ConfigureGeometryComponent(
        UStaticMeshComponent* GeometryComponent,
        UStaticMesh* CubeMesh);

    void UpdateGeometry() const;

protected:

    void OnConstruction(
        const FTransform& Transform) override;

public:

    double GetWormHeadPlaneZ() const;

    FPlane GetWormMovementPlane() const;

    FBox2D GetPlayableBounds() const;

    bool CanPlaceCircle(
        const FVector2D& Center,
        const double Radius) const;
};
