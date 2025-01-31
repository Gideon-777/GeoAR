// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "MapBorderMeshComponent.generated.h"

class UBodySetup;

/**
 * 
 */

class FMapBorderMeshSceneProxy : public FPrimitiveSceneProxy
{

public:
	FMapBorderMeshSceneProxy(const class UMapBorderMeshComponent* InComponent);

	void Init();
	virtual ~FMapBorderMeshSceneProxy();

	SIZE_T GetTypeHash() const override;

protected:
	void InitResources();

	bool IsInCollisionView(const FEngineShowFlags& EngineShowFlags) const;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const class FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual bool CanBeOccluded() const override;

protected:

	FStaticMeshVertexBuffers VertexBuffer;

	FDynamicMeshIndexBuffer32 IndexBuffer32;

	FLocalVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;

	/** The material we'll use to render this street map mesh */
	class UMaterialInterface* MaterialInterface;

	const class UMapBorderMeshComponent* MapBorderMeshComponent;
	// The Collision Response of the component being proxied
	FCollisionResponseContainer CollisionResponse;
};

/**
 * @brief GIS 오브젝트들의 외곽영역을 렌더링하기 위한 메쉬컴포넌트
 * @details GeoAR에서 사용자의 위치가 잡히고 GIS 오브젝트 데이터들이 다운로드 및 로드 \n
 *			완료되었을 때의 바운더리를 기준으로 해당 영역을 표시하는 테두리영역의 메쉬를 생성한다.\n
 *			또한 AARCameraManager에서 터치로 인한 맵이동 등에서 카메라의 위치 이동이나 줌 스케일 계산에 활용한다.
 * @author hyunwoo@realmaker.kr
 */
UCLASS(meta = (BlueprintSpawnableComponent), hidecategories = (Physics))
class GEOAR_API UMapBorderMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	

	UMapBorderMeshComponent(const class FObjectInitializer& ObjectInitializer);
	public:

	void SetDefaultMaterial(UMaterialInterface* material)
	{
		DefaultMaterial = material;
		SetMaterial(0, DefaultMaterial);
	}

	UMaterialInterface* GetDefaultMaterial() const;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	FBox2D GetInnerVerticesBounds() const;

	void OnGISLocationUpdated();

private:
	TArray<FVector> Vertices;
	FBoxSphereBounds CachedLocalBounds;
	FBox2D InnerVerticesBounds;

	UMaterialInterface* DefaultMaterial;

	friend class FMapBorderMeshSceneProxy;
};


