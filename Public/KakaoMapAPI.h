// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include <Online/HTTP/Public/Interfaces/IHttpRequest.h>
#include "KakaoMapAPI.generated.h"

#define ADDRESS_SEARCH_URL "https://dapi.kakao.com/v2/local/search/address.query="
#define KEYWORD_SEARCH_URL "https://dapi.kakao.com/v2/local/search/keyword.query="
#define KEYWORD_SEARCH_URL_ADV "https://dapi.kakao.com/v2/local/search/keyword.json?y=%s&x=%s&radius=%s" /*y = latitude, x = longitude*/
#define CATEGORY_SEARCH_URL "https://dapi.kakao.com/v2/local/search/category.%s"
#define PLACES_SEARCH_URL "https://dapi.kakao.com/v2/local/search/category.json?category_group_code=%25%20&y=%s&x=%s&radius=100"

#define KAKAO_REST_API_KEY "448edda50741aed5926540433c13d421" /*임의로 발급받은 키*/


/**
 * @brief FKakaoPlace의 리스폰스되는 결과 값의 구조체
 * @details 
 * @author hyunwoo@realmaker.kr
 */
USTRUCT(BlueprintType)
struct GEOAR_API FKakaoPlace
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString AddressName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString RoadAddressName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString CategoryGroupCode;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString CategoryGroupName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString CategoryName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString Distance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString PhoneNumber;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString PlaceName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString PlaceUrl;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString Latitude;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KakaoAPI | Place")
	FString Longitude;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlacesSearchingInBuildingDelegate, const TArray<FKakaoPlace>&, Places);


/**
 * @brief KakaoMapAPI를 사용하여 장소정보를 불러올 수 있다.
 * @details 도로주소를 사용하여 건물을 지정하여 API로 정보를 받아와야 하지만 \n
 *			리얼메이커 서버의 GIS 건물정보에 도로주소 필드가 없어 임시로 건물의 위경도를 사용하여 \n
 *			위치를 중심으로한 장소정보들이 들어오기 때문에 특정한 건물의 장소정보를 불러오지는 못한다.
 * @todo 위치정보 대신 도로명 주소를 사용하여 정확한 건물내의 Place정보를 가져와야한다.
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API UKakaoMapAPI : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "KakaoAPI", meta = (DisplayName = "GetKakaoAPI"))
	static UKakaoMapAPI* Get();
	
	UFUNCTION(BlueprintCallable, Category = "KakaoAPI")
	void RequestPlacesFromStreetmapBuilding(ABuildingCore* Building);
	/*UFUNCTION(BlueprintCallable, Category = "KakaoAPI")
	void RequestKeywordSearching(FString Keyword);*/

	UPROPERTY(BlueprintAssignable, Category = "KakaoAPI")
	FPlacesSearchingInBuildingDelegate PlacesSearchingInBuildingDelegate;
private:
	void OnBuildingSearchingPlacesReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/*void OnKeywordSearchingPlacesReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);*/
	
private:
	TArray<FKakaoPlace> Places;
	static UKakaoMapAPI* Instance;
};
