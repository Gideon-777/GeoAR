// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "MapBorderMeshActor.h"
#include "MapBorderMeshComponent.h"
#include "ObjectLocationAlignmentComponent.h"
#include "GISDataToUnrealObject.h"
#include "GISLocationMetaData.h"

// Sets default values
AMapBorderMeshActor::AMapBorderMeshActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	MapBorderComponent = CreateDefaultSubobject<UMapBorderMeshComponent>(TEXT("StreetMapComp"));
	RootComponent = MapBorderComponent;
	MapBorderComponent->SetDefaultMaterial(Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("Material'/Game/Assets/Materials/MapBorder.MapBorder'"))));
	MapBorderComponent->SetVisibility(false);
}

UMapBorderMeshComponent* AMapBorderMeshActor::GetMapBorderComponent()
{
	return MapBorderComponent;
}

// Called when the game starts or when spawned
void AMapBorderMeshActor::BeginPlay()
{
	Super::BeginPlay();
	AGISLocation* gisLocation = UGISLocationManager::AttachNewGISLocationActorToActor(this, UGISDataToUnrealObject::GetMapBorderGISLocation().X, UGISDataToUnrealObject::GetMapBorderGISLocation().Y);
	gisLocation->OnLocationUpdated.AddUObject(MapBorderComponent, &UMapBorderMeshComponent::OnGISLocationUpdated);
	gisLocation->OnLocationUpdated.Broadcast();
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("MapBorderPlaced : %f     %f"), GetActorLocation().X, GetActorLocation().Y));
}

// Called every frame
void AMapBorderMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

