// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StreetMap.h"
#include "GeoLineActor.h"
#include "GISDataToUnrealObject.generated.h"

DECLARE_STATS_GROUP(TEXT("RealMaker"), STATGROUP_RealMaker, STATCAT_Advanced);
/**
 * 
 */
class OGRGeometry;
class OGRCoordinateTransformation;
class OGRSpatialReference;
class OGRPointIterator;
class FGeoTiff;

/**
 * @brief GeoAR프로젝트에서 GIS 데이터(Json)를 받은 데이터에 있는 속성 값들을 담기 위한 구조체이다.
 * @details 
 * @see https://realmaker.atlassian.net/wiki/spaces/G/pages/413040730/GIS+API
 * @author hyunwoo@realmaker.kr
 */
struct FGISProperties
{
	FGISProperties() :
		Type(-1),
		Floor(0),
		Height(0.0f)
	{
		Name.Empty();
		SubName.Empty();
		Unique_ID.Empty();
		PakID.Empty();
	}
	int32 Type;
	FString Name;
	FString SubName;
	int32 Floor;
	float Height;
	FString Unique_ID;
	FString PakID;
	FString Usability;
};

enum ContourLineType
{
	Mountain,
	Coast,
	Isobath
};

/**
 * @brief GIS 타입의 데이터(shp, json..)를 파싱 및 액터로 생성해 언리얼 월드에서 렌더링 될 수 있도록 하는 블루프린트 클래스
 * @details 
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API UGISDataToUnrealObject : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/*UFUNCTION(BlueprintCallable,  Category = "GdalPlugin")
	static bool LoadBuildingFromShp(FString shpLayerName);
	UFUNCTION(BlueprintCallable, Category = "GdalPlugin")
	static bool LoadSeparateBuildingsFromShp(FString shpLayerName);
	UFUNCTION(BlueprintCallable,  Category = "GdalPlugin")
	static bool LoadRoadFromPolyShp(FString shpLayerName);
	UFUNCTION(BlueprintCallable,  Category = "GdalPlugin")
	static bool LoadRoadFromLineShp(FString shpLayerName);

	UFUNCTION(BlueprintCallable, Category = "GdalPlugin")
	static bool LoadSeparateBuildingsFromGeoJSON(FString shpLayerName);*/

	/** 센텀공원 DEM으로 부터 입력된 위경도의 높이값을 반환한다. RTT 프로젝트에서 사용됨. */
	UFUNCTION(BlueprintCallable, Category = "GdalPlugin")
	static bool GetDEMHeightOnCentumPark(FString Latitude, FString Longitude, float& outHeight);

	/** GeoJson 포멧의 지상 GIS 데이터로 부터 언리얼 오브젝트를 생성한다. */
	UFUNCTION(BlueprintCallable, Category = "GdalPlugin")
	static bool LoadGISObjectFromGeoJson(FString fileFullPath);

	/** GeoJson 포멧의 등심선 GIS 데이터로 부터 언리얼 오브젝트를 생성한다. */
	UFUNCTION(BlueprintCallable, Category = "GdalPlugin")
	static bool LoadIsobathFromGeoJson(FString fileFullPath);

	static FDVector2D GetMapBorderGISLocation();
	static FVector2D GetMapBorderSize();
	static float GetMapBorderThickness();

	/** 타일경계로 분리되어 있는 등고(심)선 라인을 붙여준다. */
	static void WeldAdjacentLines(TArray<FDLine>& SplitedLines);
	static void ClearLineData();
	/** GIS데이터 정보를 모두 읽어들인 후 타일경계로 쪼개진 라인데이터를 붙도록 변환하고 라인으로 렌더링되는 액터들을 생성한다. */
	static void GenerateGeoLineActorFromLoadedLineData();
private:
	/** 파싱된 GIS 데이터로 부터 Streetmap타입의 액터를 생성한다. */
	static void GenerateBuildingObject(OGRGeometry* geometry, OGRCoordinateTransformation* poCT, FStreetMapBuilding building);
	/** HrLandscape가 다운로드 가능한 영역의 Polygon(GIS개념의)를 액터로 생성한다. */
	static void GenerateDownloadableHrLandscapeArea(OGRGeometry* geometry, OGRCoordinateTransformation* poCT, FStreetMapHrLandscapeArea area);
	/** Geometry에 포함되어 있는 라인 데이터를 poCT에 설정된 좌표계로 변환하여 TArray<FDLine>타입으로 가져온다. 높이는 입력된 height로 통일된다(등고(심)선용으로 사용하기 때문)*/
	static void ExtractLineData(OGRGeometry* geometry, OGRCoordinateTransformation* poCT, TArray<FDLine>& lines, const float& height);
	/** Geometry에 포함되어 있는 라인 데이터를 poCT에 설정된 좌표계로 변환하여 TArray<FDLine>타입으로 가져온다. 높이는 입력된 DEM_Data위치에 해당하는 값으로 설정된다.)*/
	static void ExtractLineData(OGRGeometry* geometry, OGRSpatialReference* poSRS, OGRCoordinateTransformation* poCT, TArray<FDLine>& lines, FGeoTiff* DEM_Data);

	/** 입력된 라인 데이터로 부터 라인으로 렌더링 되는 액터를 생성한다. 타입에 따라 머터리얼(라인색상과 custom stencil value가 다름)이 바뀐다. */
	static void GenerateLineObject(TArray<FDLine>& lines, ContourLineType type);

	/** 입력된 파라미터 데이터들로 부터 FStreetMapRoad를 생성해 리스트로 반환한다. */
	static void AddRoadTo(TArray<FStreetMapRoad>& roads, OGRPointIterator* iter, const FGISProperties& fieldData, FDBox2D& layerBounds, FGeoTiff* pDEM_Tiff, OGRCoordinateTransformation* poCT, OGRSpatialReference* poSRS);
	static FDVector2D MapBorderGISLocation;
	static FDBox2D MapBorderGISBounds;
	static FVector2D MapBorderSize;
	static float MapBorderThickness;

	static TArray<FDLine> UnifiedRoadLines;
	static TArray<FDLine> UnifiedMountainLines;
	static TArray<FDLine> UnifiedIsobathLines;
};
