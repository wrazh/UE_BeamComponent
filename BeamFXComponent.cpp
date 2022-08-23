#include "BeamFXComponent.h"
#include "Engine.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "StaticMeshResources.h"

class FBeamFXSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	FBeamFXSceneProxy(UBeamFXComponent* InComponent) : FPrimitiveSceneProxy(InComponent), VertexFactory(GetScene().GetFeatureLevel(), "FBeamFXSceneProxy")
	{
		Comp = InComponent;

		MaterialRelevance = UMaterial::GetDefaultMaterial(MD_Surface)->GetRelevance(GetScene().GetFeatureLevel());

		FColor NewPropertyColor;
		GEngine->GetPropertyColorationColor((UObject*)InComponent, NewPropertyColor);
		SetPropertyColor(NewPropertyColor);

		StaticMeshVertexBuffers.PositionVertexBuffer.Init(1);
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.Init(1, 1);
		StaticMeshVertexBuffers.ColorVertexBuffer.Init(1);

		const FBeamFXSceneProxy* Self = this;
		ENQUEUE_RENDER_COMMAND(FMaterialSpriteSceneProxyInit)(
			[Self](FRHICommandListImmediate& RHICmdList)
		{
			Self->StaticMeshVertexBuffers.PositionVertexBuffer.InitResource();
			Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.InitResource();
			Self->StaticMeshVertexBuffers.ColorVertexBuffer.InitResource();

			FLocalVertexFactory::FDataType Data;
			Self->StaticMeshVertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(&Self->VertexFactory, Data);
			Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(&Self->VertexFactory, Data);
			Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&Self->VertexFactory, Data);
			Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(&Self->VertexFactory, Data, 0);
			Self->StaticMeshVertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(&Self->VertexFactory, Data);
			Self->VertexFactory.SetData(Data);

			Self->VertexFactory.InitResource();
		});
	}
	~FBeamFXSceneProxy()
	{
		VertexFactory.ReleaseResource();
		StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
		StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
	}
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_MaterialSpriteSceneProxy_GetDynamicMeshElements);

		const int WorstCaseVertexBufferSize = (6 * (Comp->Points.Num() - 1))* Views.Num();

		if (WorstCaseVertexBufferSize > 0)
		{
			StaticMeshVertexBuffers.PositionVertexBuffer.Init(WorstCaseVertexBufferSize);
			StaticMeshVertexBuffers.StaticMeshVertexBuffer.Init(WorstCaseVertexBufferSize, 1);
			StaticMeshVertexBuffers.ColorVertexBuffer.Init(WorstCaseVertexBufferSize);

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FMatrix matView = View->ViewMatrices.GetInvViewProjectionMatrix();
					const FVector CameraUp = -matView.GetUnitAxis(EAxis::X);
					const FVector CameraRight = -matView.GetUnitAxis(EAxis::Y);
					const FVector CameraForward = -matView.GetUnitAxis(EAxis::Z);

					TArray <FVector> Vertex;
					Vertex.AddZeroed(6 * (Comp->Points.Num() - 1));

					for (int i = 0; i < Comp->Points.Num() - 1; i++)
					{
						// Verts
						const FVector PointA = Comp->Points[i];
						const FVector PointB = Comp->Points[i + 1];

						// What I suppose to do here?
						FVector magicVector = FVector(0, 0, 1);
						magicVector.Normalize();
						magicVector *= Comp->Tickness;

						Vertex[6 * i + 0] = PointA - magicVector;
						Vertex[6 * i + 1] = PointA + magicVector;
						Vertex[6 * i + 2] = PointB - magicVector;

						Vertex[6 * i + 3] = PointA + magicVector;
						Vertex[6 * i + 4] = PointB - magicVector;
						Vertex[6 * i + 5] = PointB + magicVector;

						for (int j = 0; j < 6; j++)
							StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(6 * i + j) = Vertex[6 * i + j];

						// Tangents
						const FVector Edge01 = (Vertex[6 * i + 1] - Vertex[6 * i + 0]);
						const FVector Edge02 = (Vertex[6 * i + 2] - Vertex[6 * i + 0]);
						FVector TangentX = Edge01.GetSafeNormal();
						FVector TangentZ = (Edge02 ^ Edge01).GetSafeNormal();
						FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal();

						for (uint32 VertexIndex = 0; VertexIndex < 6; ++VertexIndex)
						{
							StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(6 * i + VertexIndex, TangentX, TangentY, TangentZ);
							StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(6 * i + VertexIndex) = FColor::White;
						}

						// UVs
						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 0, 0, FVector2D(0, 0));
						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 1, 0, FVector2D(1, 0));
						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 2, 0, FVector2D(0, 1));

						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 3, 0, FVector2D(1, 0));
						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 4, 0, FVector2D(0, 1));
						StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(6 * i + 5, 0, FVector2D(1, 1));
					}

					FMeshBatch& Mesh = Collector.AllocateMesh();

					Mesh.VertexFactory = &VertexFactory;
					Mesh.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy((View->Family->EngineShowFlags.Selection) && IsSelected(), IsHovered());
					Mesh.LCI = NULL;
					Mesh.ReverseCulling = false;
					Mesh.CastShadow = false;

					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)GetDepthPriorityGroup(View);
					Mesh.Type = PT_TriangleList;
					Mesh.bCanApplyViewModeOverrides = true;
					Mesh.bUseWireframeSelectionColoring = IsSelected();
					Mesh.bDisableBackfaceCulling = true;

					// Set up the FMeshBatchElement.
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = NULL;
					BatchElement.FirstIndex = 0;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = Vertex.Num();
					BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
					BatchElement.NumPrimitives = Vertex.Num() / 3;
					BatchElement.BaseVertexIndex = 0;

					Collector.AddMesh(ViewIndex, Mesh);
				}
			}

			FLocalVertexFactory* VertexFactoryPtr = &VertexFactory;
			const FBeamFXSceneProxy* Self = this;
			ENQUEUE_RENDER_COMMAND(FMaterialSpriteSceneProxyLegacyInit)(
				[VertexFactoryPtr, Self](FRHICommandListImmediate& RHICmdList)
			{
				Self->StaticMeshVertexBuffers.PositionVertexBuffer.UpdateRHI();
				Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.UpdateRHI();
				Self->StaticMeshVertexBuffers.ColorVertexBuffer.UpdateRHI();

				FLocalVertexFactory::FDataType Data;
				Self->StaticMeshVertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(VertexFactoryPtr, Data);
				Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactoryPtr, Data);
				Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactoryPtr, Data);
				Self->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(VertexFactoryPtr, Data, 0);
				Self->StaticMeshVertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(VertexFactoryPtr, Data);
				VertexFactoryPtr->SetData(Data);

				VertexFactoryPtr->UpdateRHI();
			});
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override { return !MaterialRelevance.bDisableDepthTest; }
	virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize() const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
	UBeamFXComponent* Comp;
private:
	mutable FStaticMeshVertexBuffers StaticMeshVertexBuffers;
	mutable FLocalVertexFactory VertexFactory;
	FMaterialRelevance MaterialRelevance;
};

UBeamFXComponent::UBeamFXComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Tickness = 4.0f;
	Points.Add(FVector(0, 0, 0));
	Points.Add(FVector(45, 45, 16));
	Points.Add(FVector(45, -45, 32));
	Points.Add(FVector(0, 0, 0));
}

FPrimitiveSceneProxy* UBeamFXComponent::CreateSceneProxy()
{
	return new FBeamFXSceneProxy(this);
}

FBoxSphereBounds UBeamFXComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox boxExtend = FBox(Points);
	FSphere sphereExtend = FBoxSphereBounds(boxExtend).GetSphere();
	return FBoxSphereBounds(LocalToWorld.GetLocation(), boxExtend.GetSize(), sphereExtend.W);
}