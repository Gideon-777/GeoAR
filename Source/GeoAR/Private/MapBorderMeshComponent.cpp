// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "MapBorderMeshComponent.h"
#include "GISDataToUnrealObject.h"
#include "ARCameraManager.h"



UMapBorderMeshComponent::UMapBorderMeshComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/*static ConstructorHelpers::FObjectFinder<UMaterialInterface> MapBorderMaterial(TEXT("Material'/Game/Assets/Materials/MapBorder.MapBorder'"));
	DefaultMaterial = MapBorderMaterial.Object;*/

	FVector2D BorderSize = UGISDataToUnrealObject::GetMapBorderSize();
	float BorderThickness = UGISDataToUnrealObject::GetMapBorderThickness();

	InnerVerticesBounds.Init();

	Vertices.Empty();

	Vertices.Add(FVector(0.f, 0.f, 0.f));

	Vertices.Add(FVector(BorderSize.X, 0.f, 0.f));

	Vertices.Add(FVector(0.f, BorderSize.Y, 0.f));

	Vertices.Add(FVector(BorderSize.X, BorderSize.Y, 0.f));

	Vertices.Add(FVector(-BorderThickness, -BorderThickness, 0.f));

	Vertices.Add(FVector(BorderSize.X + BorderThickness, -BorderThickness, 0.f));

	Vertices.Add(FVector(-BorderThickness, BorderSize.Y + BorderThickness, 0.f));

	Vertices.Add(FVector(BorderSize.X + BorderThickness, BorderSize.Y + BorderThickness, 0.f));

	CachedLocalBounds = FBoxSphereBounds(ForceInit);

	

	for (int i = 0; i < Vertices.Num(); i++)
	{
		CachedLocalBounds.BoxExtent += Vertices[i];
	}
}

UMaterialInterface* UMapBorderMeshComponent::GetDefaultMaterial() const
{
	return DefaultMaterial != nullptr ? DefaultMaterial : UMaterial::GetDefaultMaterial(MD_Surface);
}

FPrimitiveSceneProxy* UMapBorderMeshComponent::CreateSceneProxy()
{
	FMapBorderMeshSceneProxy* MapBorderMeshSceneProxy = nullptr;

	MapBorderMeshSceneProxy = new FMapBorderMeshSceneProxy(this);
	MapBorderMeshSceneProxy->Init();

	return MapBorderMeshSceneProxy;
}

FBoxSphereBounds UMapBorderMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds WorldSpaceBounds = CachedLocalBounds.TransformBy(LocalToWorld);
	WorldSpaceBounds.BoxExtent *= BoundsScale;
	WorldSpaceBounds.SphereRadius *= BoundsScale;
	return WorldSpaceBounds;
}

void UMapBorderMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	UMeshComponent::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
	if (OutMaterials.Num() == 0)
	{
		OutMaterials.Add(this->GetMaterial(0));
	}
}

FBox2D UMapBorderMeshComponent::GetInnerVerticesBounds() const
{
	return InnerVerticesBounds;
}

void UMapBorderMeshComponent::OnGISLocationUpdated()
{
	InnerVerticesBounds.Init();
	for (int i = 4; i < Vertices.Num(); i++)
	{
		InnerVerticesBounds += FVector2D(Vertices[i]) + FVector2D(GetAttachmentRootActor()->GetActorLocation());
	}
	UARCoreDelegateSet::OnInitializedMapBorderBoundsDelegate.Broadcast(InnerVerticesBounds);
}

FMapBorderMeshSceneProxy::FMapBorderMeshSceneProxy(const class UMapBorderMeshComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, VertexFactory(GetScene().GetFeatureLevel(), "FMapBorderMeshSceneProxy")
	, MapBorderMeshComponent(InComponent)
{
	
}

void FMapBorderMeshSceneProxy::Init()
{
	

	TArray<FDynamicMeshVertex> DynamicVertices;
	DynamicVertices.SetNumUninitialized(8);

	for (int i = 0; i < 8; i++)
	{
		DynamicVertices[i].Position = MapBorderMeshComponent->Vertices[i];
	}

	FVector2D BorderSize = UGISDataToUnrealObject::GetMapBorderSize();
	float BorderThickness = UGISDataToUnrealObject::GetMapBorderThickness();

	float UvsOffset = BorderThickness / BorderSize.X;

	DynamicVertices[0].Color = FColor::White;
	DynamicVertices[0].TextureCoordinate[0] = FVector2D(UvsOffset, 1.f);

	DynamicVertices[1].Color = FColor::White;
	DynamicVertices[1].TextureCoordinate[0] = FVector2D(1.f - UvsOffset, 1.f);

	DynamicVertices[2].Color = FColor::White;
	DynamicVertices[2].TextureCoordinate[0] = FVector2D(1.f - UvsOffset, 1.f);

	DynamicVertices[3].Color = FColor::White;
	DynamicVertices[3].TextureCoordinate[0] = FVector2D(UvsOffset, 1.f);

	DynamicVertices[4].Color = FColor::White;
	DynamicVertices[4].TextureCoordinate[0] = FVector2D(0.f, 0.f);

	DynamicVertices[5].Color = FColor::White;
	DynamicVertices[5].TextureCoordinate[0] = FVector2D(1.f, 0.f);

	DynamicVertices[6].Color = FColor::White;
	DynamicVertices[6].TextureCoordinate[0] = FVector2D(1.f, 0.f);

	DynamicVertices[7].Color = FColor::White;
	DynamicVertices[7].TextureCoordinate[0] = FVector2D(0.f, 0.f);

	IndexBuffer32.Indices.SetNumUninitialized(24);

	IndexBuffer32.Indices[0] = 4;
	IndexBuffer32.Indices[1] = 0;
	IndexBuffer32.Indices[2] = 5;
							    
	IndexBuffer32.Indices[3] = 0;
	IndexBuffer32.Indices[4] = 1;
	IndexBuffer32.Indices[5] = 5;
							    
	IndexBuffer32.Indices[6] = 5;
	IndexBuffer32.Indices[7] = 1;
	IndexBuffer32.Indices[8] = 3;
							    
	IndexBuffer32.Indices[9] = 5;
	IndexBuffer32.Indices[10] = 3;
	IndexBuffer32.Indices[11] = 7;
							    
	IndexBuffer32.Indices[12] = 7;
	IndexBuffer32.Indices[13] = 3;
	IndexBuffer32.Indices[14] = 6;
							    
	IndexBuffer32.Indices[15] = 3;
	IndexBuffer32.Indices[16] = 2;
	IndexBuffer32.Indices[17] = 6;
							    
	IndexBuffer32.Indices[18] = 6;
	IndexBuffer32.Indices[19] = 2;
	IndexBuffer32.Indices[20] = 0;
							    
	IndexBuffer32.Indices[21] = 6;
	IndexBuffer32.Indices[22] = 0;
	IndexBuffer32.Indices[23] = 4;

	MaterialInterface = nullptr;
	this->MaterialRelevance = MapBorderMeshComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());

	VertexBuffer.InitFromDynamicVertex(&VertexFactory, DynamicVertices);

	// Enqueue initialization of render resource
	InitResources();

	// Set a material
	{
		if (MapBorderMeshComponent->GetNumMaterials() > 0)
		{
			MaterialInterface = MapBorderMeshComponent->GetMaterial(0);
		}

		// Use the default material if we don't have one set
		if (MaterialInterface == nullptr)
		{
			MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}
}

FMapBorderMeshSceneProxy::~FMapBorderMeshSceneProxy()
{
	VertexBuffer.PositionVertexBuffer.ReleaseResource();
	VertexBuffer.StaticMeshVertexBuffer.ReleaseResource();
	VertexBuffer.ColorVertexBuffer.ReleaseResource();
	IndexBuffer32.ReleaseResource();
	VertexFactory.ReleaseResource();
}

SIZE_T FMapBorderMeshSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FMapBorderMeshSceneProxy::InitResources()
{
	BeginInitResource(&VertexBuffer.PositionVertexBuffer);
	BeginInitResource(&VertexBuffer.StaticMeshVertexBuffer);
	BeginInitResource(&VertexBuffer.ColorVertexBuffer);
	BeginInitResource(&IndexBuffer32);
	BeginInitResource(&VertexFactory);
}

bool FMapBorderMeshSceneProxy::IsInCollisionView(const FEngineShowFlags& EngineShowFlags) const
{
	return  EngineShowFlags.CollisionVisibility || EngineShowFlags.CollisionPawn;
}

void FMapBorderMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
{
	const int IndexCount = IndexBuffer32.Indices.Num();
	if (VertexBuffer.PositionVertexBuffer.GetNumVertices() > 0 && IndexCount > 0)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			const FSceneView& View = *Views[ViewIndex];

			const bool bIsWireframe = AllowDebugViewmodes() && View.Family->EngineShowFlags.Wireframe;

			FColoredMaterialRenderProxy* WireframeMaterialRenderProxyOrNull = GEngine->WireframeMaterial && bIsWireframe ? new FColoredMaterialRenderProxy(GEngine->WireframeMaterial->GetRenderProxy(), FLinearColor(0, 0.5f, 1.f)) : NULL;


			//if (MustDrawMeshDynamically(View))
			{
				const bool bInCollisionView = IsInCollisionView(ViewFamily.EngineShowFlags);
				const bool bCanDrawCollision = bInCollisionView && IsCollisionEnabled();

				if (!IsCollisionEnabled() && bInCollisionView)
				{
					continue;
				}

				// Draw the mesh!
				FMeshBatch& Mesh = Collector.AllocateMesh();
				
				FMaterialRenderProxy* MaterialProxy = NULL;
				if (WireframeMaterialRenderProxyOrNull != nullptr)
				{
					MaterialProxy = WireframeMaterialRenderProxyOrNull;
				}
				else
				{
					if (bCanDrawCollision)
					{
						MaterialProxy = new FColoredMaterialRenderProxy(GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(), FColor::Cyan);
					}
					else if (MaterialProxy == nullptr)
					{
						MaterialProxy = MapBorderMeshComponent->GetDefaultMaterial()->GetRenderProxy();
					}
				}

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer32;
				Mesh.bWireframe = WireframeMaterialRenderProxyOrNull != nullptr;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				Mesh.CastShadow = false;

				bool bHasPrecomputedVolumetricLightmap;
				FMatrix PreviousLocalToWorld;
				int32 SingleCaptureIndex;
				bool bOutputVelocity;
				GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, false, DrawsVelocity(), false);
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexCount / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffer.PositionVertexBuffer.GetNumVertices() - 1;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;

				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}
}

FPrimitiveViewRelevance FMapBorderMeshSceneProxy::GetViewRelevance(const class FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);

	const bool bAlwaysHasDynamicData = false;

	// Only draw dynamically if we're drawing in wireframe or we're selected in the editor
	Result.bDynamicRelevance = true;// MustDrawMeshDynamically(*View) || bAlwaysHasDynamicData;
	Result.bStaticRelevance = false;// !MustDrawMeshDynamically(*View);
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();

	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}

uint32 FMapBorderMeshSceneProxy::GetMemoryFootprint(void) const
{
	return sizeof(*this) + GetAllocatedSize();
}

bool FMapBorderMeshSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}
