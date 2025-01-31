// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#include "GISDataToUnrealObject.h"
#include "gdal_priv.h"
#include "Engine/Engine.h"
#include "ogrsf_frmts.h"
#include "ogr_spatialref.h"
#include "ogr_feature.h"
#include "GdalPluginBPLibrary.h"
#include "StreetMapActor.h"
#include "StreetMap.h"
#include "StreetMapComponent.h"
#include "AREventManager.h"
#include "DoubleTypeStruct.h"
#include "RmUtil.h"
#include "Polygon2D.h"

DECLARE_CYCLE_STAT(TEXT("RealMaker GisData Cycle time check"), STAT_GISLoadingCycleCheck, STATGROUP_RealMaker);
DECLARE_CYCLE_STAT(TEXT("RealMaker GeoPackage Cycle time check"), STAT_GeoPackageLoadTime, STATGROUP_RealMaker);
DECLARE_CYCLE_STAT(TEXT("RealMaker GeoJSON Cycle time check"), STAT_GeoJSONLoadTime, STATGROUP_RealMaker);

static uint32 totalBuilidngCount = 0;
static uint32 passedBuildingCount = 0;
static uint32 totalRoadCount = 0;
bool UGISDataToUnrealObject::GetDEMHeightOnCentumPark(FString Latitude, FString Longitude, float& outHeight)
{
	FString gdalDataOutPath;
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if PLATFORM_ANDROID
	extern FString GExternalFilePath;

	gdalDataOutPath = GExternalFilePath + TEXT("/GDAL_DATA/");
#elif PLATFORM_WINDOWS
	gdalDataOutPath.Append(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	gdalDataOutPath.Append(TEXT("GDAL_DATA/"));
#endif
	static FGeoTiff CentumDemTiff(gdalDataOutPath + TEXT("CentumParkDEM.tif"));
	
	OGRSpatialReference epsg4326;
	epsg4326.importFromEPSG(4326);

	

	FDVector2D InPoint;
	
	//////////////////////////////////////////////////////////////////////////
	FPolygon2D azWorks4FloorBounds;
	azWorks4FloorBounds.AddPoint(FVector2D(211387.81f, 286322.02f));
	azWorks4FloorBounds.AddPoint(FVector2D(211375.38f, 286340.36f));
	azWorks4FloorBounds.AddPoint(FVector2D(211392.37f, 286347.54f));
	azWorks4FloorBounds.AddPoint(FVector2D(211405.39f, 286328.16f));
	OGRSpatialReference epsg5187;
	epsg5187.importFromEPSG(5187);
	OGRCoordinateTransformation *pOCTForExternalOSR = OGRCreateCoordinateTransformation(&epsg4326, &epsg5187);
	InPoint.X = FCString::Atod(*Longitude);
	InPoint.Y = FCString::Atod(*Latitude);
	pOCTForExternalOSR->Transform(1, &InPoint.X, &InPoint.Y);
	if (azWorks4FloorBounds.IsPointInPolygon(FVector2D(InPoint.X, InPoint.Y)))
	{
		outHeight = 21.72f;
		return true;
	}
	//////////////////////////////////////////////////////////////////////////
	FPolygon2D demBounds;
	demBounds.AddPoint(FVector2D(1147881.5f, 1687696.5f));
	demBounds.AddPoint(FVector2D(1148065.2f, 1687399.8f));
	demBounds.AddPoint(FVector2D(1147974.7f, 1687340.6f));
	demBounds.AddPoint(FVector2D(1147813.1f, 1687654.2f));

	OGRSpatialReference demOSR = CentumDemTiff.GetMeta().SRS;
	pOCTForExternalOSR = OGRCreateCoordinateTransformation(&epsg4326, &demOSR);	
	InPoint.X = FCString::Atod(*Longitude);
	InPoint.Y = FCString::Atod(*Latitude);
	pOCTForExternalOSR->Transform(1, &InPoint.X, &InPoint.Y);
	if (!demBounds.IsPointInPolygon(FVector2D(InPoint.X, InPoint.Y)))
	{
		outHeight = 6.5f;
		return true;
	}
	static bool bReadRasterBand = false;
	if (bReadRasterBand == false)
	{
		CentumDemTiff.ReadRasterBand();
		bReadRasterBand = true;
	}
	outHeight = CentumDemTiff.GetHeight(CentumDemTiff.GetMeta().SRS, InPoint.X, InPoint.Y);
	//CentumDemTiff.ReleaseRasterBand();

	if (outHeight == CentumDemTiff.GetNoDataValue())
	{
		outHeight = 6.5f;
		return true;
	}
	else
	{
		return true;
	}

	
}

bool UGISDataToUnrealObject::LoadGISObjectFromGeoJson(FString fileFullPath)
{
	GDALDataset* poDS = nullptr;
	poDS = static_cast<GDALDataset*>(GDALOpenEx(TCHAR_TO_ANSI(*fileFullPath), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));

	if (poDS == nullptr)
	{
#if PLATFORM_ANDROID
		UE_LOG(GDAL, Log, TEXT("Shapefile Open failed."));
#endif

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("ShapeFile open failed!"));
		}
		return false;
	}
#if PLATFORM_ANDROID
	UE_LOG(GDAL, Log, TEXT("Shapefile Open Succeed."));
#endif

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("ShapeFile open success!"));

	}

	if (!poDS)
	{
		return false;
	}

	OGRLayer  *poLayer = poDS->GetLayer(0);
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	OGRSpatialReference* poSRS = poLayer->GetSpatialRef();

	if (!poLayer || !poFDefn || !poSRS)
	{
		return false;
	}

	UGdalPluginBPLibrary::CheckBesselEllipsoid(poSRS);

	OGRCoordinateTransformation *poCT;
	poCT = OGRCreateCoordinateTransformation(poSRS, UGdalPluginBPLibrary::GetTargetOSR());

	if (GEngine && !poCT)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Failed to create OGRCoordinateTransformation."));
		return false;
	}

	//////////////////////

	/*
	// DEM 임시코드
	*/
	UGISDataToUnrealObject::UnifiedRoadLines.Empty();
	FString gdalDataOutPath;
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if PLATFORM_ANDROID
	extern FString GExternalFilePath;

	gdalDataOutPath = GExternalFilePath + TEXT("/GDAL_DATA/");
#elif PLATFORM_WINDOWS
	gdalDataOutPath.Append(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	gdalDataOutPath.Append(TEXT("GDAL_DATA/"));
#endif
	FGeoTiff CentumDemTiff(gdalDataOutPath + TEXT("CentumDEM.tif"));
	CentumDemTiff.ReadRasterBand();
	/*
	// DEM 임시코드 END
	*/
	poLayer->ResetReading();
	OGRFeature *poFeature;

	UStreetMap* streetMapObjForRoads = NewObject<UStreetMap>();
	UStreetMap* streetMapObjForRoadCenterLines = NewObject<UStreetMap>();
	UStreetMap* streetMapObjForBuilding = NewObject<UStreetMap>();

	TArray<FStreetMapRoad>& Roads = streetMapObjForRoads->GetRoads();
	TArray<FStreetMapRoad>& RoadCenterLines = streetMapObjForRoadCenterLines->GetRoads();
	TArray<FStreetMapBuilding>& buildings = streetMapObjForBuilding->GetBuildings();

	TArray<FDLine> mountainContourLines;
	TArray<FDLine> coastContourLines;

	FDBox2D layerBounds(ForceInit);

	while ((poFeature = poLayer->GetNextFeature()) != nullptr)
	{
		FGISProperties properties;

		for (int iField = 0; iField < poFDefn->GetFieldCount(); iField++)
		{
			FString fieldName(poFDefn->GetFieldDefn(iField)->GetNameRef());
			if (fieldName.Compare(TEXT("prt")) == 0)
			{
				properties.Type = poFeature->GetFieldAsInteger(iField);
			}
			else if (fieldName.Compare(TEXT("bld_nm")) == 0)
			{
				properties.Name = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
			}
			else if (fieldName.Compare(TEXT("bld_nm_dc")) == 0)
			{
				properties.SubName = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
			}
			else if (fieldName.Compare(TEXT("grnd_flr")) == 0)
			{
				properties.Floor = poFeature->GetFieldAsInteger(iField);
			}
			else if (fieldName.Compare(TEXT("height")) == 0)
			{
				properties.Height = (float)poFeature->GetFieldAsDouble(iField);
			}
			else if (fieldName.Compare(TEXT("bd_mgt_sn")) == 0)
			{
				properties.Unique_ID = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
			}
			else if (fieldName.Compare(TEXT("pak_id")) == 0)
			{
				properties.PakID = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
			}
			else if (fieldName.Compare(TEXT("usability")) == 0)
			{
				properties.Usability = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
			}
			else
			{
				//Test Code
				if (fieldName.Compare(TEXT("ALWNC_DE")) == 0)
				{
					properties.Type = 11;
					//continue;
				}
				else if (fieldName.Compare(TEXT("RN")) == 0) // ���θ�
				{
					properties.Name = FString(UTF8_TO_TCHAR(poFeature->GetFieldAsString(iField)));
					//continue;
				}
				else if (fieldName.Compare(TEXT("ROAD_BT")) == 0) // ������
				{
					properties.Height = (float)poFeature->GetFieldAsDouble(iField);
					if (properties.Height < 1.f)
					{
						properties.Height = 1.f;
					}
				}
				else if (fieldName.Compare(TEXT("RDS_DPN_SE")) == 0) //���� 0�̸� ���θ��ּ� ���ε���, 1�̸� ������ ����� ���굵��
				{
					properties.Floor = poFeature->GetFieldAsInteger(iField); //���굵�� �ʵ尡 ���� ������ �ӽ÷� Floor�� ���.
				}
				//Test Code End

				//break;
			}
		}

		// properties.Type
		// 11 : �����߽ɼ�
		// 1 : 도로
		// 2 : 건물
		// 3 : 해안선
		// 4 : 등고선
		// 10 : 고해상도 랜드스케이프 영역 폴리곤
		if (properties.Type == -1)
		{
			OGRFeature::DestroyFeature(poFeature);
			continue;
		}
		OGRGeometry *poGeometry = poFeature->GetGeometryRef();
		switch (properties.Type)
		{
		case 1:
		{
			FStreetMapRoad road;

			if (poGeometry != NULL)
			{
				switch (poGeometry->getGeometryType())
				{
					case wkbPolygon:
					{
						OGRPolygon *poPolygon = (OGRPolygon *)poGeometry;
						OGRPointIterator* iter = poPolygon->getExteriorRingCurve()->getPointIterator();
						OGRPoint point;
						while (iter->getNextPoint(&point))
						{
							double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
							double demHeight = CentumDemTiff.GetHeight(*poSRS, targetOSR_X, targetOSR_Y);
							if (demHeight == CentumDemTiff.NoDataValue)
							{
								demHeight = 0;
							}

							poCT->Transform(1, &targetOSR_X, &targetOSR_Y);
							MapBorderGISBounds += FDVector2D(targetOSR_X, targetOSR_Y);
							layerBounds += FDVector2D(targetOSR_X, targetOSR_Y);
							road.RoadRawPoints.Add(FDVector(targetOSR_X, targetOSR_Y, demHeight));
							road.RoadType = EStreetMapRoadType::GISPolygon;
						}
						Roads.Add(road);
					}
						break;
				}
			}
		}
		break;
		case 2:
		{
			FStreetMapBuilding building;

			building.BuildingName = properties.Name;
			building.BuildingLevels = properties.Floor;
			building.Height = properties.Height;
			building.UsabilityCode = properties.Usability;
			///////////////////////////////// �ӽ��ڵ� /////////////////////////////////
			/*totalBuilidngCount++;

			if (building.Height != 0 && building.Height < 18)
			{
				passedBuildingCount++;
				continue;
			}
			if (building.BuildingLevels != 0 && building.BuildingLevels < 6)
			{
				passedBuildingCount++;
				continue;
			}*/
			//////////////////////////////////////////////////////////////////////////

			if (poGeometry != NULL)
			{
				switch (poGeometry->getGeometryType())
				{
				case wkbMultiPolygon:
				{
					OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon *)poGeometry;
					for (int i = 0; i < poMultiPolygon->getNumGeometries(); i++)
					{
						OGRPolygon *poPolygon = (OGRPolygon*)poMultiPolygon->getGeometryRef(i);
						OGRPointIterator* iter = poPolygon->getExteriorRingCurve()->getPointIterator();
						OGRPoint point;
						//UStreetMap* streetMapObj = NewObject<UStreetMap>();
						FDBox2D buildingsBounds(ForceInit);
						FDLine2D BuildingPoints;
						while (iter->getNextPoint(&point))
						{
							double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
							poCT->Transform(1, &targetOSR_X, &targetOSR_Y);
							layerBounds += FDVector2D(targetOSR_X, targetOSR_Y);
							buildingsBounds += FDVector2D(targetOSR_X, targetOSR_Y);
							building.BuildingRawPoints.Add(FDVector2D(targetOSR_X, targetOSR_Y));
						}
						UGdalPluginBPLibrary::TargetPrjXY2LatLong(buildingsBounds.GetCenter(), building.Latitude, building.Longitude);

						building.Height *= 100.f;

						float demHeight = CentumDemTiff.GetHeight(*UGdalPluginBPLibrary::GetTargetOSR(), buildingsBounds.GetCenter().X, buildingsBounds.GetCenter().Y);
						if (demHeight == CentumDemTiff.NoDataValue)
						{
							demHeight = 0;
						}
						demHeight *= 100.f;
						building.StandingHeightOnGround = demHeight;
						
						buildings.Add(building);
					}
				}
				break;
				}
			}
		}
		break;
		case 3:
		{
			ExtractLineData(poGeometry, poCT, coastContourLines, properties.Height);
		}
		break;
		case 4:
		{
			ExtractLineData(poGeometry, poCT, mountainContourLines, properties.Height);
		}
		break;
		case 10:
		{
			FStreetMapHrLandscapeArea area;
			area.HrLandscapeName = properties.Name;
			area.PakID = properties.PakID;

			GenerateDownloadableHrLandscapeArea(poGeometry, poCT, area);
		}
		break;
			
		case 11:
		{
			FString CurrentGeometryTypeName = poGeometry->getGeometryName();
			switch (poGeometry->getGeometryType())
			{
			case wkbLineString:
			{
				OGRLineString* poLineString = (OGRLineString*)poGeometry;
				OGRPointIterator* iter = poLineString->getPointIterator();
				AddRoadTo(RoadCenterLines, iter, properties, layerBounds, &CentumDemTiff, poCT, poSRS);
				totalRoadCount++;
			}
			break;
			case wkbMultiLineString:
			{
				OGRMultiLineString *poMulitLineString = (OGRMultiLineString *)poGeometry;
				for (int i = 0; i < poMulitLineString->getNumGeometries(); i++)
				{
					OGRLineString* poLineString = (OGRLineString*)poMulitLineString->getGeometryRef(i);
					OGRPointIterator* iter = poLineString->getPointIterator();
					AddRoadTo(RoadCenterLines, iter, properties, layerBounds, &CentumDemTiff, poCT, poSRS);
					totalRoadCount++;
				}
			}
			break;
			default:
				ensure(0);
				break;
			}
		}
		break;
		default:
			break;
		}
		
		OGRFeature::DestroyFeature(poFeature);
	}

	FDVector pivot;

	pivot.X = layerBounds.GetCenter().X;
	pivot.Y = layerBounds.GetCenter().Y;
	pivot.Z = 0;

	UWorld* world = GetWorldStatic();

	////////////////////////////////  Generate Roads(Polygon Type)  //////////////////////////////////////////

	for (auto& road : Roads)
	{
		for (auto& vec : road.RoadRawPoints)
		{
			vec -= pivot;
			vec.Y *= -1.f;
			vec *= 100.f /* meterToCentimeter */;
		}			
	}

	for (int i = 0; i < Roads.Num(); i++)
	{
		Roads[i].RoadPoints.Append(FDVectorHelper::FDLineToFLine(Roads[i].RoadRawPoints));
	}

	streetMapObjForRoads->SetProjectedPivotXY(FDVector2D(pivot.X, pivot.Y));

	AStreetMapActor* streetMapRoadActor = world->SpawnActor<AStreetMapActor>();
	streetMapRoadActor->SetStreetMap(streetMapObjForRoads);

	////////////////////////////////  Generate Road Centerlines  /////////////////////////////////////////

	for (auto& centerLine : RoadCenterLines)
	{
		for (auto& vec : centerLine.RoadRawPoints)
		{
			vec -= pivot;
			vec.Y *= -1;
			vec *= 100;
		}
	}
	for (int i = 0; i < RoadCenterLines.Num(); i++)
	{
		RoadCenterLines[i].RoadPoints.Append(FDVectorHelper::FDLineToFLine(RoadCenterLines[i].RoadRawPoints));
	}

	streetMapObjForRoadCenterLines->SetProjectedPivotXY(FDVector2D(pivot.X, pivot.Y));


	AStreetMapActor* streetMapRoadCenterLineActor = world->SpawnActor<AStreetMapActor>();

	streetMapRoadCenterLineActor->SetStreetMap(streetMapObjForRoadCenterLines);
	////////////////////////////////  Generate Builidings  //////////////////////////////////////////
	for (auto& building : buildings)
	{
		for (auto& vec : building.BuildingRawPoints)
		{
			vec -= FDVector2D(pivot.X, pivot.Y);
			vec.Y *= -1.f;
			vec *= 100.f /* meterToCentimeter */;
		}
	}

	for (int i = 0; i < buildings.Num(); i++)
	{
		buildings[i].BuildingPoints.Append(FDVectorHelper::FDLine2DToFLine2D(buildings[i].BuildingRawPoints));
	}
	streetMapObjForBuilding->SetProjectedPivotXY(FDVector2D(pivot.X, pivot.Y));

	AStreetMapActor* streetMapBuildingActor = world->SpawnActor<AStreetMapActor>();
	streetMapBuildingActor->SetStreetMap(streetMapObjForBuilding);

	////////////////////////////////  Add contour line data  //////////////////////////////////////////

	if (coastContourLines.Num() != 0)
	{
		//UnifiedIsobathLines += coastContourLines;
		GenerateLineObject(coastContourLines, Coast);
	}
	if (mountainContourLines.Num() != 0)
	{
		//UnifiedMountainLines += mountainContourLines;
		GenerateLineObject(mountainContourLines, Mountain);
	}
	CentumDemTiff.ReleaseRasterBand();
	GDALClose(poDS);
	return true;
}

bool UGISDataToUnrealObject::LoadIsobathFromGeoJson(FString fileFullPath)
{
	GDALDataset* poDS = nullptr;
	poDS = static_cast<GDALDataset*>(GDALOpenEx(TCHAR_TO_ANSI(*fileFullPath), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));

	if (poDS == nullptr)
	{
#if PLATFORM_ANDROID
		UE_LOG(GDAL, Log, TEXT("Shapefile Open failed."));
#endif

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("ShapeFile open failed!"));
		}
		return false;
	}
#if PLATFORM_ANDROID
	UE_LOG(GDAL, Log, TEXT("Shapefile Open Succeed."));
#endif

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("ShapeFile open success!"));

	}

	if (!poDS)
	{
		return false;
	}

	OGRLayer  *poLayer = poDS->GetLayer(0);
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	OGRSpatialReference* poSRS = poLayer->GetSpatialRef();

	if (!poLayer || !poFDefn || !poSRS)
	{
		return false;
	}

	UGdalPluginBPLibrary::CheckBesselEllipsoid(poSRS);

	OGRCoordinateTransformation *poCT;
	poCT = OGRCreateCoordinateTransformation(poSRS, UGdalPluginBPLibrary::GetTargetOSR());

	if (GEngine && !poCT)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Failed to create OGRCoordinateTransformation."));
		return false;
	}

	//////////////////////
	poLayer->ResetReading();
	OGRFeature *poFeature;

	TArray<FDLine> IsobathLines;
	while ((poFeature = poLayer->GetNextFeature()) != nullptr)
	{
		float height(0);

		for (int iField = 0; iField < poFDefn->GetFieldCount(); iField++)
		{
			height = static_cast<float>(poFeature->GetFieldAsDouble(iField));
		}

		OGRGeometry *poGeometry = poFeature->GetGeometryRef();

		ExtractLineData(poGeometry, poCT, IsobathLines, height);

		OGRFeature::DestroyFeature(poFeature);
	}

	if (IsobathLines.Num() != 0)
	{
		GenerateLineObject(IsobathLines, Isobath);
	}

	GDALClose(poDS);

	return true;
}

FDVector2D UGISDataToUnrealObject::GetMapBorderGISLocation()
{
	MapBorderGISLocation.X = MapBorderGISBounds.Min.X;
	MapBorderGISLocation.Y = MapBorderGISBounds.Max.Y;
	return MapBorderGISLocation;
}

FVector2D UGISDataToUnrealObject::GetMapBorderSize()
{
	MapBorderSize.X = (MapBorderGISBounds.Max.X - MapBorderGISBounds.Min.X) * 100.f;
	MapBorderSize.Y = (MapBorderGISBounds.Max.Y - MapBorderGISBounds.Min.Y) * 100.f;
	return MapBorderSize;
}

float UGISDataToUnrealObject::GetMapBorderThickness()
{
	return MapBorderThickness;
}

void UGISDataToUnrealObject::WeldAdjacentLines(TArray<FDLine>& SplitedLines)
{
	int32 lastIdx = SplitedLines.Num() - 1;

	TArray<int32> reservationListToRemove;

	for (int32 i = 0; i <= lastIdx; i++)
	{
		FDLine& lhs = SplitedLines[i];
		for (int32 j = i+1; j <= lastIdx; j++)
		{
			FDLine& rhs = SplitedLines[j];

			if (lhs[0] == rhs.Last())
			{
				lhs.RemoveAt(0);
				if (lhs.Last() == rhs[0])
				{
					lhs.RemoveAt(lhs.Num() - 1);
				}
				rhs.Append(lhs);
				SplitedLines.RemoveAt(i);
				WeldAdjacentLines(SplitedLines);
				return;
			}
			else if (lhs.Last() == rhs[0])
			{
				rhs.RemoveAt(0);

				lhs.Append(rhs);
				SplitedLines.RemoveAt(j);
				WeldAdjacentLines(SplitedLines);
				return;
			}
		}
	}
}

void UGISDataToUnrealObject::ClearLineData()
{
	UnifiedMountainLines.Empty();
	UnifiedIsobathLines.Empty();
}

void UGISDataToUnrealObject::GenerateGeoLineActorFromLoadedLineData()
{
	if (UnifiedIsobathLines.Num() != 0)
	{
		WeldAdjacentLines(UnifiedIsobathLines);
		GenerateLineObject(UnifiedIsobathLines, Coast);
	}
	if (UnifiedMountainLines.Num() != 0)
	{
		WeldAdjacentLines(UnifiedMountainLines);
		GenerateLineObject(UnifiedMountainLines, Mountain);
	}
}

void UGISDataToUnrealObject::GenerateBuildingObject(OGRGeometry* poGeometry, OGRCoordinateTransformation* poCT, FStreetMapBuilding building)
{
	if (poGeometry != NULL)
	{
		switch (poGeometry->getGeometryType())
		{
		case wkbMultiPolygon:
		{
			OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon *)poGeometry;
			for (int i = 0; i < poMultiPolygon->getNumGeometries(); i++)
			{
				OGRPolygon *poPolygon = (OGRPolygon*)poMultiPolygon->getGeometryRef(i);
				OGRPointIterator* iter = poPolygon->getExteriorRingCurve()->getPointIterator();
				OGRPoint point;
				UStreetMap* streetMapObj = NewObject<UStreetMap>();
				TArray<FStreetMapBuilding>& buildings = streetMapObj->GetBuildings();
				FDBox2D buildingsBound(ForceInit);
				FDLine2D BuildingPoints;
				while (iter->getNextPoint(&point))
				{
					double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
					poCT->Transform(1, &targetOSR_X, &targetOSR_Y);
					buildingsBound += FDVector2D(targetOSR_X, targetOSR_Y);
					BuildingPoints.Add(FDVector2D(targetOSR_X, targetOSR_Y));
				}
				FDVector2D pivot;
				pivot.X = buildingsBound.GetCenter().X;
				pivot.Y = buildingsBound.GetCenter().Y;

				for (auto& vec : BuildingPoints)
				{
					vec -= pivot;
					vec.Y *= -1.f;
					vec *= 100.f /* meterToCentimeter */;
				}
				building.Height *= 100.f;

				building.BuildingPoints.Append(FDVectorHelper::FDLine2DToFLine2D(BuildingPoints));

				buildings.Add(building);

				streetMapObj->SetProjectedPivotXY(pivot);

				UWorld* world = GetWorldStatic();
				AStreetMapActor* streetMapActor = world->SpawnActor<AStreetMapActor>();
				streetMapActor->SetStreetMap(streetMapObj);

				UStreetMapComponent* component = streetMapActor->GetStreetMapComponent();
				//component->UFID = UFID;
				component->BuildingName = building.BuildingName;
			}
		}
		break;
		case wkbPolygon:
		{
			OGRPolygon *poPolygon = (OGRPolygon *)poGeometry;
			OGRPointIterator* iter = poPolygon->getExteriorRingCurve()->getPointIterator();
			OGRPoint point;
			UStreetMap* streetMapObj = NewObject<UStreetMap>();
			TArray<FStreetMapBuilding>& buildings = streetMapObj->GetBuildings();
			FDBox2D buildingsBound(ForceInit);
			FDLine2D BuildingPoints;
			while (iter->getNextPoint(&point))
			{
				double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
				poCT->Transform(1, &targetOSR_X, &targetOSR_Y);
				buildingsBound += FDVector2D(targetOSR_X, targetOSR_Y);
				BuildingPoints.Add(FDVector2D(targetOSR_X, targetOSR_Y));
				//MapBorderGISBounds += FDVector2D(targetOSR_X, targetOSR_Y);
			}
			
			building.Height *= 100.f;

			building.BuildingPoints.Append(FDVectorHelper::FDLine2DToFLine2D(BuildingPoints));

			buildings.Add(building);

			UWorld* world = GetWorldStatic();
			AStreetMapActor* streetMapActor = world->SpawnActor<AStreetMapActor>();
			streetMapActor->SetStreetMap(streetMapObj);

			UStreetMapComponent* component = streetMapActor->GetStreetMapComponent();
			component->BuildingName = building.BuildingName;
		}
		break;
		}
	}
}

void UGISDataToUnrealObject::GenerateDownloadableHrLandscapeArea(OGRGeometry* poGeometry, OGRCoordinateTransformation* poCT, FStreetMapHrLandscapeArea area)
{
	if (poGeometry != NULL)
	{
		switch (poGeometry->getGeometryType())
		{
		/*case wkbMultiPolygon:
		{
			OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon *)geometry;

		}
		break;*/
		case wkbPolygon:
		{
			OGRPolygon *poPolygon = (OGRPolygon *)poGeometry;
			OGRPointIterator* iter = poPolygon->getExteriorRingCurve()->getPointIterator();
			OGRPoint point;
			UStreetMap* streetMapObj = NewObject<UStreetMap>();
			TArray<FStreetMapHrLandscapeArea>& areas = streetMapObj->GetHrLandscapeAreas();
			FDBox2D areaBounds(ForceInit);
			FDLine2D areaPoints;
			while (iter->getNextPoint(&point))
			{
				double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
				poCT->Transform(1, &targetOSR_X, &targetOSR_Y);
				areaBounds += FDVector2D(targetOSR_X, targetOSR_Y);
				areaPoints.Add(FDVector2D(targetOSR_X, targetOSR_Y));
				//MapBorderGISBounds += FDVector2D(targetOSR_X, targetOSR_Y);
			}
			FDVector2D pivot;
			pivot.X = areaBounds.GetCenter().X;
			pivot.Y = areaBounds.GetCenter().Y;

			for (auto& vec : areaPoints)
			{
				vec -= pivot;
				vec.Y *= -1.f;
				vec *= 100.f /* meterToCentimeter */;
			}

			area.Points.Append(FDVectorHelper::FDLine2DToFLine2D(areaPoints));

			areas.Add(area);

			streetMapObj->SetProjectedPivotXY(pivot);

			UWorld* world = GetWorldStatic();
			AStreetMapActor* streetMapActor = world->SpawnActor<AStreetMapActor>();
			streetMapActor->SetStreetMap(streetMapObj);
		}
		break;
		}
	}
}

void UGISDataToUnrealObject::ExtractLineData(OGRGeometry* geometry, OGRCoordinateTransformation* poCT, TArray<FDLine>& lines, const float& height)
{
	FString CurrentGeometryTypeName = geometry->getGeometryName();
	switch (geometry->getGeometryType())
	{
		case wkbLineString:
		{
			OGRLineString* poLineString = (OGRLineString*)geometry;
			OGRPointIterator* iter = poLineString->getPointIterator();
			OGRPoint point;
			TArray<FDVector> vectorList;
			while (iter->getNextPoint(&point))
			{
				double coordX(point.getX()), coordY(point.getY());
				poCT->Transform(1, &coordX, &coordY);

				vectorList.Add(FDVector(coordX, coordY, height));
				MapBorderGISBounds += FDVector2D(coordX, coordY);
			}
			lines.Add(vectorList);
		}
		break;
		case wkbMultiLineString:
		{
			OGRMultiLineString *poMulitLineString = (OGRMultiLineString *)geometry;
			for (int i = 0; i < poMulitLineString->getNumGeometries(); i++)
			{
				OGRLineString* poLineString = (OGRLineString*)poMulitLineString->getGeometryRef(i);
				OGRPointIterator* iter = poLineString->getPointIterator();
				OGRPoint point;
				TArray<FDVector> vectorList;
				while (iter->getNextPoint(&point))
				{
					double coordX(point.getX()), coordY(point.getY());
					poCT->Transform(1, &coordX, &coordY);

					vectorList.Add(FDVector(coordX, coordY, height));
					MapBorderGISBounds += FDVector2D(coordX, coordY);
				}
				lines.Add(vectorList);
			}
		}
		break;
		default:
			ensure(0);
			break;
	}
	
}

void UGISDataToUnrealObject::ExtractLineData(OGRGeometry* geometry, OGRSpatialReference* poSRS, OGRCoordinateTransformation* poCT, TArray<FDLine>& lines, FGeoTiff* DEM_Data)
{
	if (!DEM_Data)
	{
		return;
	}
	FString CurrentGeometryTypeName = geometry->getGeometryName();
	switch (geometry->getGeometryType())
	{
	case wkbLineString:
	{
		OGRLineString* poLineString = (OGRLineString*)geometry;
		OGRPointIterator* iter = poLineString->getPointIterator();
		OGRPoint point;
		TArray<FDVector> vectorList;
		while (iter->getNextPoint(&point))
		{
			double coordX(point.getX()), coordY(point.getY());
			
			double targetOSR_X(point.getX()), targetOSR_Y(point.getY());
			double demHeight = DEM_Data->GetHeight(*poSRS, coordX, coordY);
			if (demHeight == DEM_Data->NoDataValue)
			{
				demHeight = 0;
			}

			poCT->Transform(1, &coordX, &coordY);

			vectorList.Add(FDVector(coordX, coordY, demHeight));
			MapBorderGISBounds += FDVector2D(coordX, coordY);
		}
		lines.Add(vectorList);
	}
	break;
	/*case wkbMultiLineString:
	{
		OGRMultiLineString *poMulitLineString = (OGRMultiLineString *)geometry;
		for (int i = 0; i < poMulitLineString->getNumGeometries(); i++)
		{
			OGRLineString* poLineString = (OGRLineString*)poMulitLineString->getGeometryRef(i);
			OGRPointIterator* iter = poLineString->getPointIterator();
			OGRPoint point;
			TArray<FDVector> vectorList;
			while (iter->getNextPoint(&point))
			{
				double coordX(point.getX()), coordY(point.getY());
				poCT->Transform(1, &coordX, &coordY);

				vectorList.Add(FDVector(coordX, coordY, height));
				MapBorderGISBounds += FDVector2D(coordX, coordY);
			}
			lines.Add(vectorList);
		}
	}
	break;*/
	default:
		ensure(0);
		break;
	}
}

void UGISDataToUnrealObject::GenerateLineObject(TArray<FDLine>& lines, ContourLineType type)
{
	FDBox LocalBox(ForceInit);

	for (const FDLine& line : lines)
	{
		LocalBox += FDBox(line);
	}



	FDVector pivot;
	pivot.X = LocalBox.Min.X;
	pivot.Y = LocalBox.Max.Y;
	pivot.Z = 0;

	for (FDLine& line : lines)
	{
		for (FDVector& vec : line)
		{
			vec -= pivot;
			vec.Y *= -1;
			vec *= 100;
		}
	}

	UWorld* world = GetWorldStatic();
	AGeoLineActor* geoLineActor = world->SpawnActor<AGeoLineActor>();

	switch (type)
	{
	case Coast:
		//geoLineActor->CreateIsoBathLineMesh(lines, pivot.X, pivot.Y, true, /*FLinearColor(*/FVector(0.f, 1.f, 1.f)/*)*/); break;
		geoLineActor->CreateLineMesh(lines, pivot.X, pivot.Y, false, /*FLinearColor(*/FVector(0.f, 1.f, 1.f)/*)*/); break;
	case Mountain:
		geoLineActor->CreateLineMesh(lines, pivot.X, pivot.Y, true, FVector(0.f, 1.f, 0.f)); break;
	case Isobath:
		geoLineActor->CreateIsoBathLineMesh(lines, pivot.X, pivot.Y, false, /*FLinearColor(*/FVector(0.f, 1.f, 0.3f)/*)*/); break;
	default: break;
	}
}

void UGISDataToUnrealObject::AddRoadTo(TArray<FStreetMapRoad>& roads, OGRPointIterator* iter, const FGISProperties& fieldData, FDBox2D& layerBounds, FGeoTiff* pDEM_Tiff, OGRCoordinateTransformation* poCT, OGRSpatialReference* poSRS)
{
	ensure(iter != nullptr);

	OGRPoint point;
	FStreetMapRoad road;

	while (iter->getNextPoint(&point))
	{
		double coordX(point.getX()), coordY(point.getY());

		double demHeight(0);

		if (pDEM_Tiff)
		{
			demHeight = pDEM_Tiff->GetHeight(*poSRS, coordX, coordY);
			if (demHeight == pDEM_Tiff->NoDataValue)
			{
				demHeight = 0;
			}
		}

		poCT->Transform(1, &coordX, &coordY);
		MapBorderGISBounds += FDVector2D(coordX, coordY);
		layerBounds += FDVector2D(coordX, coordY);

		road.RoadRawPoints.Add(FDVector(coordX, coordY, demHeight));
	}
	road.RoadName = fieldData.Name;
	road.RoadType = EStreetMapRoadType::RoadCenterLine;
	road.RoadWidth = fieldData.Height;
	road.bIsBranchRoad = fieldData.Floor != 0 ? true : false;
	roads.Add(road);
}

FDVector2D UGISDataToUnrealObject::MapBorderGISLocation = FDVector2D(0, 0);

FDBox2D UGISDataToUnrealObject::MapBorderGISBounds = FDBox2D(ForceInit);

FVector2D UGISDataToUnrealObject::MapBorderSize = FVector2D(300000.f, 300000.f);

float UGISDataToUnrealObject::MapBorderThickness = 10000.f;

TArray<FDLine> UGISDataToUnrealObject::UnifiedRoadLines;

TArray<FDLine> UGISDataToUnrealObject::UnifiedMountainLines;

TArray<FDLine> UGISDataToUnrealObject::UnifiedIsobathLines;
