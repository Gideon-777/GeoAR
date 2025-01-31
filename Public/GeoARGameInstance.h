// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GeoARGameInstance.generated.h"


UCLASS()
class GEOAR_API UGeoARGameInstance : public UGameInstance
{
	GENERATED_BODY()	

public: 
	UGeoARGameInstance();
	
public:		
	virtual void Shutdown() override;
protected:
	virtual void OnStart() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Realmaker|ReplaySystem")
	void StartRecording();
	UFUNCTION(BlueprintCallable, Category = "Realmaker|ReplaySystem")
	void StopRecording();
	UFUNCTION(BlueprintCallable, Category = "Realmaker|ReplaySystem")
	void StartReplay();
	UFUNCTION(BlueprintPure, Category = "Realmaker|ReplaySystem")
	bool IsReplaying();

	void SetReplayController(class AGeoARReplayController* replayController);
protected:
	UPROPERTY(BlueprintReadOnly)
	AGeoARReplayController* ReplayController;
private:
	UPROPERTY(EditDefaultsOnly, Category = "Realmaker|ReplaySystem")
	FString RecordingName;

	UPROPERTY(EditDefaultsOnly, Category = "Realmaker|ReplaySystem")
	FString FriendlyRecordingName;

	

	bool bIsReplaying = false;

};
