// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Engine/Texture2D.h>
#include "AndroidServiceBPLibrary.h"
#include "FusionPositioningSystemBPLibrary.h"
#include "RmUtil.h"
#include "ReplayRecorder.generated.h"

/**
 * @brief IMU 센서 및 RTT 신호데이터를 기록하기 위한 클래스
 * @details EKF(Extened Kalman Filter)를 이용한 RTT 모듈을 구현하기 위해 Android 디바이스의 센서를 레코딩하기 위해 만든 클래스이다.
 *
 * @todo RttFilterUpdate()와 DistributionFilterUpdate()는 RTT의 위치를 필터링하여 안정적인 위치를 제공하는 용도로 만들어졌는데 다른 곳으로 이동이 필요하다.
 * @see UFusionPositioningSystemBPLibrary \n
 *		https://realmaker.atlassian.net/wiki/spaces/~881307840/pages/1057751075 (사용된 EKF 논문 설명)\n
 *		https://realmaker.atlassian.net/wiki/spaces/~881307840/pages/1182629971/RTT+Distribution+Filter (Distribution Filter 설명)
		
 * @author hyunwoo@realmaker.kr
 */
UCLASS()
class GEOAR_API AAnroidSensorRecorder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAnroidSensorRecorder();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(NetMulticast, Reliable)
	void RecordRttRangingResults(const FString& results);

	UFUNCTION(NetMulticast, Reliable)
	void RecordAccelometerEvent(const FIMUSensor& Event);

	UFUNCTION(NetMulticast, Reliable)
	void RecordGyroEvent(const FIMUSensor& Event);

	UFUNCTION(NetMulticast, Reliable)
	void RecordGameRotationVectorEvent(const FGameRotationVector& Event);
private:
	bool RttFilterUpdate(TArray<FRangingResult>& RangingResults);
	bool DistributionFilterUpdate(TArray<FRangingResult>& RangingResults, float* OutDistribution = nullptr, bool bRecursive = false);
private:
	bool bIsReplaying;

	TMap<FString, FSimpleKalmanFilter> KF_FilteredRangingData;
	TMap<FString, FRangingResult> PrevRangingDataList;
	TArray<FString> FilteredAPs;
	int64 PrevRangingTimestamp;
	int64 currentTimestamp;
};