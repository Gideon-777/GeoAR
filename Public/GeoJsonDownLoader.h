// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "DoubleTypeStruct.h"
#include "RmUtil.h"
#include "GeoJsonDownLoader.generated.h"

#define GIS_FILE_DOWNLOAD_URL TEXT("http://211.181.226.218:8080/gj/")
#define GIS_FILELIST_REQUEST_URL TEXT("http://211.181.226.218:8080/dx/getgeojsonlist_in_range?lat=%f&lon=%f&range=%d")
#define ISOBATH_FILE_DOWNLOAD_URL TEXT("http://211.181.226.218:8080/tifjson/")
#define ISOBATH_FILELIST_REQUEST_URL TEXT("http://211.181.226.218:8080/dx/gettiflist?lat=%f&lon=%f")
//#define LRLANDSCAPE_FILEHASH_REQUEST_URL TEXT("http://211.181.226.218:/??/")
#define DEFAULT_GIS_FILE_SAVE_FOLDER TEXT("/GDATA/BASEMAP/")
#define ISOBATH_FILE_SAVE_FOLDER TEXT("/GDATA/ISOBATH/")

#define GIS_LOAD_RANGE_KILOMETER 1





//USTRUCT(BlueprintType)
//struct GEOAR_API FGeojsonFiles
//{
//	GENERATED_USTRUCT_BODY()
//	
//	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealMaker|GeoJsonDownloader")
//	TArray<FString> Files;
//};

DECLARE_MULTICAST_DELEGATE(FDefaultGISFilesLoadedDelegate);
DECLARE_MULTICAST_DELEGATE(FIsobathFilesLoadedDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAllGISDataLoadedDelegate);

/**
 * @brief GIS데이터(Json타입의)를 리얼메이커 서버로 부터 받기 위한 클래스
 * @details 다운로드 완료후 액터스폰까지 여기서 이루어진다.
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API UGeoJsonDownLoader : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
	UFUNCTION(BlueprintPure, Category = "RealMaker|GeoJsonDownloader")
	static UGeoJsonDownLoader* GetGeoJsonDownLoaderRef() {
		return thisPtr;
	}
	
	UFUNCTION(BlueprintCallable, Category = "RealMaker|GeoJsonDownloader")
	static void RequestAndLoadGISDataFile(FVector2D latlong, uint8 desiredLoadRangeKilometer = 3);

	UFUNCTION(BlueprintCallable, Category = "RealMaker|GeoJsonDownloader")
	static void RequestAndLoadGISDataFile2(FString latlong, uint8 desiredLoadRangeKilometer = 3);

	UPROPERTY(BlueprintAssignable, Category = "RealMaker|GeoJsonDownloader")
	FAllGISDataLoadedDelegate AllGISDataLoadedDelegate;

	FDefaultGISFilesLoadedDelegate DefaultGISFilesLoadedDelegate;
	FDefaultGISFilesLoadedDelegate IsobathFilesLoadedDelegate;
private:
	void OnDefaultGISFileListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnAllGISDataFileDownloadCompleted();
	void OnDefaultGISFilesLoaded();

	void OnIsobathFileListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnAllIsobathDataFileDownloadCompleted();
	void OnIsobathFilesLoaded();

	void SpawnMapBorderActor();
private:
	static UGeoJsonDownLoader* thisPtr;

	FMultipleFileDownloader DefaultGISFileDownloader;
	FMultipleFileDownloader IsobathFileDownloader;

	bool bDefaultObjectLoaded;
	bool bIsobathLoaded;
};
