// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#include "GeoJsonDownLoader.h"
#include "GISDataToUnrealObject.h"
#include "GdalPluginBPLibrary.h"
#include "MapBorderMeshActor.h"
#include "CoreGlobals.h"
#include "RmUtil.h"

UGeoJsonDownLoader* UGeoJsonDownLoader::thisPtr = nullptr;

UGeoJsonDownLoader::UGeoJsonDownLoader(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultGISFileDownloader(FString(TEXT("GISDataDownload01")))
	, IsobathFileDownloader(FString(TEXT("GISDataDownload01")))
	, bDefaultObjectLoaded(false)
	, bIsobathLoaded(false)
{
	thisPtr = this;
	DefaultGISFilesLoadedDelegate.AddUObject(this, &UGeoJsonDownLoader::OnDefaultGISFilesLoaded);
	IsobathFilesLoadedDelegate.AddUObject(this, &UGeoJsonDownLoader::OnIsobathFilesLoaded);
}

void UGeoJsonDownLoader::RequestAndLoadGISDataFile(FVector2D latlong, uint8 desiredLoadRangeKilometer)
{
	thisPtr->bIsobathLoaded = false;
	thisPtr->bDefaultObjectLoaded = false;
	//Request Default Object GeoJson FileList
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	//This is the url on which to process the request

	uint8 possibleRange = FMath::CeilToInt((desiredLoadRangeKilometer - 1) * 0.5f);
	possibleRange = FMath::Clamp<uint8>(possibleRange, 0, 10);

	Request->SetURL(FString::Printf(GIS_FILELIST_REQUEST_URL, latlong.X, latlong.Y, possibleRange));
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindUObject(thisPtr, &UGeoJsonDownLoader::OnDefaultGISFileListReceived);
	Request->ProcessRequest();

	//Request Isobath GeoJson FileList
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request2 = FHttpModule::Get().CreateRequest();
	//This is the url on which to process the request
	Request2->SetURL(FString::Printf(ISOBATH_FILELIST_REQUEST_URL, latlong.X, latlong.Y));
	Request2->SetVerb("GET");
	Request2->OnProcessRequestComplete().BindUObject(thisPtr, &UGeoJsonDownLoader::OnIsobathFileListReceived);
	Request2->ProcessRequest();
}

void UGeoJsonDownLoader::RequestAndLoadGISDataFile2(FString latlong, uint8 desiredLoadRangeKilometer)
{
	TArray<FString> parsedArray;

	latlong.ParseIntoArray(parsedArray, TEXT(", "));
	if (parsedArray.Num() != 2)
	{
		return;
	}
	double Lat = FCString::Atod(parsedArray[0].GetCharArray().GetData());
	double Long = FCString::Atod(parsedArray[1].GetCharArray().GetData());

	FVector2D latlongVector2D(Lat, Long);

	RequestAndLoadGISDataFile(latlongVector2D, desiredLoadRangeKilometer);
}

void UGeoJsonDownLoader::OnAllGISDataFileDownloadCompleted()
{
	UGISDataToUnrealObject::ClearLineData();
	check(DefaultGISFileDownloader.GetFileListDownloadFailed().Num() == 0);
	for (FString fileName : DefaultGISFileDownloader.GetDownloadCompletedFileListIncludeFullPath())
	{
		UGISDataToUnrealObject::LoadGISObjectFromGeoJson(fileName);
	}
	/* test code */
	/*/////////////////////////////////////////////////////////////////////
	*/

	FString CentumRoadCenterLinePath;
#if PLATFORM_ANDROID
	extern FString GExternalFilePath;

	CentumRoadCenterLinePath = GExternalFilePath + TEXT("/GDAL_DATA/");
#elif PLATFORM_WINDOWS
	CentumRoadCenterLinePath.Append(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	CentumRoadCenterLinePath.Append(TEXT("GDAL_DATA/"));
#endif
	CentumRoadCenterLinePath.Append(TEXT("TestData/Centum_StreetAddress.shp"));
	UGISDataToUnrealObject::LoadGISObjectFromGeoJson(CentumRoadCenterLinePath);

	/*
	*//////////////////////////////////////////////////////////////////////
	//UGISDataToUnrealObject::GenerateGeoLineActorFromLoadedLineData();
	DefaultGISFilesLoadedDelegate.Broadcast();
}

void UGeoJsonDownLoader::OnDefaultGISFileListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		return;
	}
	
	if (Response->GetResponseCode() != 200)
	{
		ensure(false);
		return;
	}
	TArray<FString> downloadFilelist;
	
	downloadFilelist.Empty();

	FString FilelistString = Response->GetContentAsString();
	FilelistString = FilelistString.LeftChop(1);
	FilelistString = FilelistString.RightChop(1);
	FilelistString.ParseIntoArray(downloadFilelist, TEXT(","));

	for (FString& fileName : downloadFilelist)
	{
		fileName = fileName.LeftChop(1);
		fileName = fileName.RightChop(1);
	}

	
	DefaultGISFileDownloader.AllFileDownloadCompleted.BindUObject(this, &UGeoJsonDownLoader::OnAllGISDataFileDownloadCompleted);
	DefaultGISFileDownloader.DownloadFiles(GIS_FILE_DOWNLOAD_URL, DEFAULT_GIS_FILE_SAVE_FOLDER, downloadFilelist);
}

void UGeoJsonDownLoader::OnDefaultGISFilesLoaded()
{
	bDefaultObjectLoaded = true;

	if (bDefaultObjectLoaded && bIsobathLoaded)
	{
		AllGISDataLoadedDelegate.Broadcast();
		SpawnMapBorderActor();
	}
}

void UGeoJsonDownLoader::SpawnMapBorderActor()
{
	GetWorldStatic()->SpawnActor<AMapBorderMeshActor>();
}

void UGeoJsonDownLoader::OnIsobathFileListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		ensure(false);
		return;
	}
	TArray<FString> downloadFilelist;

	downloadFilelist.Empty();

	FString FilelistString = Response->GetContentAsString();
	FilelistString = FilelistString.LeftChop(1);
	FilelistString = FilelistString.RightChop(1);
	FilelistString.ParseIntoArray(downloadFilelist, TEXT(","));

	for (FString& fileName : downloadFilelist)
	{
		fileName = fileName.LeftChop(5);
		fileName = fileName.RightChop(1);
		fileName.Append(TEXT(".json"));
	}

	IsobathFileDownloader.AllFileDownloadCompleted.BindUObject(this, &UGeoJsonDownLoader::OnAllIsobathDataFileDownloadCompleted);
	IsobathFileDownloader.DownloadFiles(ISOBATH_FILE_DOWNLOAD_URL, ISOBATH_FILE_SAVE_FOLDER, downloadFilelist);
}

void UGeoJsonDownLoader::OnAllIsobathDataFileDownloadCompleted()
{
	check(IsobathFileDownloader.GetFileListDownloadFailed().Num() == 0);
	for (FString fileName : IsobathFileDownloader.GetDownloadCompletedFileListIncludeFullPath())
	{
		UGISDataToUnrealObject::LoadIsobathFromGeoJson(fileName);
	}
	IsobathFilesLoadedDelegate.Broadcast();
}

void UGeoJsonDownLoader::OnIsobathFilesLoaded()
{
	bIsobathLoaded = true;

	if (bDefaultObjectLoaded && bIsobathLoaded)
	{
		AllGISDataLoadedDelegate.Broadcast();
		//SpawnMapBorderActor();
	}
}


