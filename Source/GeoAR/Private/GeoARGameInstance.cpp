// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "GeoARGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Internationalization/Culture.h"
#include "FusionPositioningSystemModule.h"
#include "FusionPositioningSystemDefine.h"

UGeoARGameInstance::UGeoARGameInstance()
{
	RecordingName = TEXT("GeoAR_Replay");
	FriendlyRecordingName = TEXT("GeoAR Replay");
	ReplayController = nullptr;
}

void UGeoARGameInstance::OnStart()
{	
	if (IFusionPositioningSystemModule::IsAvailable())
	{
		IFusionPositioningSystem* System = IFusionPositioningSystemModule::Get().GetSystem();
		if (System != nullptr)
		{
			System->InitSystem();
		}
	}
}

void UGeoARGameInstance::Shutdown()
{
	if (IFusionPositioningSystemModule::IsAvailable())
	{
		IFusionPositioningSystem* System = IFusionPositioningSystemModule::Get().GetSystem();
		if (System != nullptr)
		{
			System->EndSystem();
		}
	}
}

void UGeoARGameInstance::StartRecording()
{
	GEngine->AddOnScreenDebugMessage(102, 5, FColor::White, FString(TEXT("Started Replay Recording")));
	StartRecordingReplay(RecordingName, FriendlyRecordingName);
}

void UGeoARGameInstance::StopRecording()
{
	GEngine->AddOnScreenDebugMessage(102, 5, FColor::White, FString(TEXT("Stoped Replay Recording")));
	StopRecordingReplay();
}

void UGeoARGameInstance::StartReplay()
{
	GEngine->AddOnScreenDebugMessage(102, 5, FColor::White, FString(TEXT("PlayReplay")));
	ReplayController = nullptr;
	PlayReplay(RecordingName, nullptr);
	bIsReplaying = true;
}

bool UGeoARGameInstance::IsReplaying()
{
	return bIsReplaying;
}

void UGeoARGameInstance::SetReplayController(class AGeoARReplayController* replayController)
{
	ReplayController = replayController;
}
