// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DepthGuideActor.generated.h"

class UWidgetComponent;
class UUserWidget;
class UTextBlock;

/**
 * @brief 수심지형(Landscape)에 깊이정보를 UI 레벨에 표시하기위한 클래스
 * @details
 * @bug ARCamera와 동기화 문제로 UI가 떨리는 듯한 증상이 나타난다.
 * @see https://realmaker.atlassian.net/wiki/spaces/AR/pages/244187163/AR+UI \n
 *		링크문서의 1~4번 시안에만 해당
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API ADepthGuideActor : public AActor
{
	GENERATED_UCLASS_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GuideXInterval"), Category = "DepthGuide")
	float GuideXIntervalMeter = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GuidXNumber"), Category = "DepthGuide")
	int32 GuideXNumber = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DepthInfoUIPadding"), Category = "DepthGuide")
	int32 DepthInfoUIPadding = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "bWidgetSizeLoaded"), Category = "DepthGuide")
	bool bWidgetSizeLoaded;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Material"), Category = "DepthGuide")
	UMaterialInterface* material;
	
private:
	TArray<UWidgetComponent*> GuideXWidgetList;
	TMap< UWidgetComponent*, UTextBlock* > GuideInfoMap;

	TSubclassOf<class UUserWidget> DepthGuideXClass;
	TSubclassOf<class UUserWidget> DepthInfoUIClass;

	FIntPoint PrevViewportSize;

	FIntPoint GetCurrentViewportSize();
	void UpdateDepthInfoUILayout();
	void UpdateDepthInfoData();
	FVector2D GetWidgetPosition(UUserWidget* textWidget);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
