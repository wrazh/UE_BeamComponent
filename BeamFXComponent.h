#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "BeamFXComponent.generated.h"

UCLASS() 
class UBeamFXComponent : public UMeshComponent
{
	GENERATED_BODY()

public:
	UBeamFXComponent(const FObjectInitializer& ObjectInitializer);
	TArray<FVector> Points;
	float Tickness;
protected:
	virtual class FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
};