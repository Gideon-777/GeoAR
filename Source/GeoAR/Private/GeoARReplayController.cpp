// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "GeoARReplayController.h"
#include <Engine/DemoNetDriver.h>
#include "../Public/GeoARGameInstance.h"
#include <Kismet/GameplayStatics.h>
#include <Engine/LevelStreamingDynamic.h>

AGeoARReplayController::AGeoARReplayController() :
	PrevCameraTexture(nullptr),
	CameraTexture(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AGeoARReplayController::RestartRecording()
{
	if (UWorld* World = GetWorld())
	{
		if (UDemoNetDriver* DemoDriver = World->GetDemoNetDriver())
		{
			FLatentActionInfo LatentInfo;
			for (FName LevelName : LoadedDynamicLevelNames)
			{
				UGameplayStatics::UnloadStreamLevel(this, LevelName, LatentInfo, false);
			}
			LoadedDynamicLevelNames.Empty();
			DemoDriver->GotoTimeInSeconds(0.f);
		}
	}
}

void AGeoARReplayController::BeginPlay()
{
	Super::BeginPlay();
	
	//GetWorldSettings()->SetPauserPlayerState(PlayerState);
	if (UWorld* World = GetWorld())
	{
		if (UDemoNetDriver* DemoDriver = World->GetDemoNetDriver())
		{
			DemoDriver->GotoTimeInSeconds(0.f);
		}
	}
	//GetWorldSettings()->SetPauserPlayerState();
}

void AGeoARReplayController::PostInitProperties()
{
	Super::PostInitProperties();
	if (UWorld* World = GetWorld())
	{
		if (UGeoARGameInstance* GameInstance = Cast<UGeoARGameInstance>(World->GetGameInstance()))
		{
			GameInstance->SetReplayController(this);
		}
	}
}

void AGeoARReplayController::Destroyed()
{
	Super::Destroyed();
	if (PrevCameraTexture)
	{
		PrevCameraTexture->ConditionalBeginDestroy();
		PrevCameraTexture = nullptr;
	}
	if (CameraTexture)
	{
		CameraTexture->ConditionalBeginDestroy();
		CameraTexture = nullptr;
	}
}
