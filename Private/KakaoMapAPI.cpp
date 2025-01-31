// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "KakaoMapAPI.h"
#include "ObjectLocationAlignmentComponent.h"
#include "BuildingCoreComponent.h"
#include <Online/HTTP/Public/Interfaces/IHttpRequest.h>
#include <Online/HTTP/Public/HttpModule.h>
#include <Online/HTTP/Public/Interfaces/IHttpResponse.h>

UKakaoMapAPI* UKakaoMapAPI::Get()
{
	if (Instance == nullptr || !Instance->IsValidLowLevel() || Instance->IsPendingKill())
	{
		Instance = Cast<UKakaoMapAPI>(UKakaoMapAPI::StaticClass()->GetDefaultObject());
	}
	return Instance;
}

void UKakaoMapAPI::RequestPlacesFromStreetmapBuilding(ABuildingCore* Building)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(FString::Printf(TEXT(PLACES_SEARCH_URL), *Building->Latitude, *Building->Longitude));
	Request->SetVerb("GET");
	Request->SetHeader("Authorization", FString(TEXT("KakaoAK ")) + KAKAO_REST_API_KEY);
	Request->SetHeader("Content-Type", TEXT("application/json"));

	Request->OnProcessRequestComplete().BindUObject(this, &UKakaoMapAPI::OnBuildingSearchingPlacesReceived);
	Request->ProcessRequest();
}

//void UKakaoMapAPI::RequestKeywordSearching(FString Keyword)
//{
//	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
//
//	Request->SetURL(KEYWORD_SEARCH_URL + Keyword);
//	Request->SetVerb("GET");
//	Request->SetHeader("Authorization", FString(TEXT("KakaoAK ")) + KAKAO_REST_API_KEY);
//
//	Request->OnProcessRequestComplete().BindUObject(this, &UKakaoMapAPI::OnKeywordSearchingPlacesReceived);
//}

void UKakaoMapAPI::OnBuildingSearchingPlacesReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		return;
	}

	FString Result = Response->GetContentAsString();

	if (Response->GetResponseCode() != 200)
	{
		ensure(false);
		return;
	}
	Places.Empty();

	TSharedPtr<FJsonObject> JsonObject;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		TArray<TSharedPtr<FJsonValue>> documents = JsonObject->GetArrayField("documents");

		for (TSharedPtr<FJsonValue> value : documents)
		{
			const TSharedPtr<FJsonObject>& obj = value->AsObject();
			FKakaoPlace place;
			place.AddressName			= obj->GetStringField("adress_name");
			place.CategoryGroupCode		= obj->GetStringField("category_group_code");
			place.CategoryGroupName		= obj->GetStringField("category_group_name");
			place.CategoryName			= obj->GetStringField("category_name");
			place.Distance				= obj->GetStringField("distance");
			place.Id					= obj->GetStringField("id");
			place.Latitude				= obj->GetStringField("y");
			place.Longitude				= obj->GetStringField("x");
			place.PhoneNumber			= obj->GetStringField("phone");
			place.PlaceName				= obj->GetStringField("place_name");
			place.PlaceUrl				= obj->GetStringField("place_url");
			place.RoadAddressName		= obj->GetStringField("road_address_name");

			Places.Add(place);
		}
		PlacesSearchingInBuildingDelegate.Broadcast(Places);
	}
}

//void UKakaoMapAPI::OnKeywordSearchingPlacesReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
//{
//
//}

UKakaoMapAPI* UKakaoMapAPI::Instance = nullptr;
