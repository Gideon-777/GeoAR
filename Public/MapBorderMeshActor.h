// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapBorderMeshActor.generated.h"

class UMapBorderMeshComponent;

/**
 * @brief UMapBorderMeshComponent를 가지는 액터 클래스
 * @details 
 * @see UMapBorderMeshComponent
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API AMapBorderMeshActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapBorderMeshActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StreetMap")
	UMapBorderMeshComponent* MapBorderComponent;

	UMapBorderMeshComponent* GetMapBorderComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
