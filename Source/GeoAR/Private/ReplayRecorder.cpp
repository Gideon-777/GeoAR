// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.


#include "ReplayRecorder.h"
#include <Kismet/GameplayStatics.h>
#include <Engine/Texture.h>
#include <TextureResource.h>
#include <AugmentedReality/Public/ARBlueprintLibrary.h>
#include <AugmentedReality/Public/ARTextures.h>
#include "FusionPositioningSystemBPLibrary.h"
#include "RmUtil.h"
#include <Json/Public/Dom/JsonObject.h>
#include <Json/Public/Dom/JsonValue.h>
#include <Json/Public/Serialization/JsonReader.h>
#include <Json/Public/Serialization/JsonSerializer.h>
#include "AndroidServiceImpl.h"
#include "AndroidServiceBPLibrary.h"
#include "DoubleTypeStruct.h"
THIRD_PARTY_INCLUDES_START
#include <Eigen/Sparse>
#include <Eigen/Core>
#include <Eigen/Dense>

THIRD_PARTY_INCLUDES_END

#define VALID_DISTRIBUTION 1.0F

// Sets default values
AAnroidSensorRecorder::AAnroidSensorRecorder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

// Called when the game starts or when spawned
void AAnroidSensorRecorder::BeginPlay()
{
	Super::BeginPlay();
	bIsReplaying = GetWorld()->IsPlayingReplay() ? true : false;
	PrevRangingTimestamp = -1;
	currentTimestamp = -1;
	bReplicates = true;

	URTTDelegate::GetRTTDelegate()->DelegateForRttRecording.BindUObject(this, &AAnroidSensorRecorder::RecordRttRangingResults);

	if (UAndroidServiceBPLibrary::GetAndroidServiceImpl())
	{
		UAndroidServiceBPLibrary::GetAndroidServiceImpl()->DelegateForAccRecording.BindUObject(this, &AAnroidSensorRecorder::RecordAccelometerEvent);
		UAndroidServiceBPLibrary::GetAndroidServiceImpl()->DelegateForGyroRecording.BindUObject(this, &AAnroidSensorRecorder::RecordGyroEvent);
		UAndroidServiceBPLibrary::GetAndroidServiceImpl()->DelegateForGameRotationRecording.BindUObject(this, &AAnroidSensorRecorder::RecordGameRotationVectorEvent);
	}
}

// Called every frame
void AAnroidSensorRecorder::Tick(float DeltaTime)
{
	return;
}

bool AAnroidSensorRecorder::RttFilterUpdate(TArray<FRangingResult>& RangingResults)
{
	FilteredAPs.Empty();

	TArray<FString> ToRemovePrevApIDs;
	TArray<FString> ToAddPrevApIDs;

	for (TPair<FString, FRangingResult> pair : PrevRangingDataList)
	{
		const FRangingResult& prevRangingResult = pair.Value;
		int64 timestamp = FCString::Atoi64(*prevRangingResult.Timestamp);
		if (currentTimestamp > 0 && (currentTimestamp - timestamp) > 600)
		{
			ToRemovePrevApIDs.Add(prevRangingResult.ApId);
		}
		else if (FRangingResult* foundData = RangingResults.FindByPredicate([pair](const FRangingResult& other) {return other.ApId == pair.Key; }))
		{
			//GEngine->AddOnScreenDebugMessage(130, 0.1f, FColor::Red, FString::Printf(TEXT("%s"), *pair.Key), true, FVector2D(4, 4));

		}
		else
		{
			RangingResults.Add(pair.Value);
			GEngine->AddOnScreenDebugMessage(131, 60, FColor::Blue, FString::Printf(TEXT("Recovered Prev Ap Rannging : %s"), *pair.Key), true, FVector2D(4, 4));
		}
		
	}

	for (const FString& RemoveID : ToRemovePrevApIDs)
	{
		PrevRangingDataList.Remove(RemoveID);
		KF_FilteredRangingData.Remove(RemoveID);
	}

	if (!DistributionFilterUpdate(RangingResults))
	{
		return false;
	}

	for (const FString& FilteredAP_ID : FilteredAPs)
	{
		if (const FRangingResult* foundValue = PrevRangingDataList.Find(FilteredAP_ID))
		{
			RangingResults.Add(*foundValue);
		}
	}

	for (FRangingResult& rangingResult : RangingResults)
	{
		FSimpleKalmanFilter& Kalman = KF_FilteredRangingData.FindOrAdd(rangingResult.ApId);
		rangingResult.Distance = Kalman.Update(rangingResult.Distance);
	}

	//PrevRangingDataList.Empty();
	for (FRangingResult& rangingResult : RangingResults)
	{
		FRangingResult& prevData = PrevRangingDataList.FindOrAdd(rangingResult.ApId);
		prevData = rangingResult;
		int64 timestamp = FCString::Atoi64(*rangingResult.Timestamp);
		if (PrevRangingTimestamp < timestamp)
		{
			PrevRangingTimestamp = timestamp;
		}
	}

	return true;
}

bool AAnroidSensorRecorder::DistributionFilterUpdate(TArray<FRangingResult>& RangingResults, float* OutDistribution, bool bRecursive)
{
	int N = RangingResults.Num();

	if (N < 3)
	{
		return false;
	}

	Eigen::MatrixXd A(N - 1, 2);
	Eigen::VectorXd b(N - 1);

	FDBox2D CoordArea;

	TArray<FDVector2D> ApLocations;
	for (const FRangingResult& result : RangingResults)
	{
		FDVector2D ApLocation(result.LocX, result.LocY);
		ApLocations.Add(ApLocation);
		CoordArea += ApLocation;
	}

	for (FDVector2D& Loc : ApLocations)
	{
		Loc -= CoordArea.Min;
	}

	for (int idx = 1; idx < N; ++idx)
	{
		A(idx - 1, 0) = 2.0f * (ApLocations[idx].X - ApLocations[0].X);
		A(idx - 1, 1) = 2.0f * (ApLocations[idx].Y - ApLocations[0].Y);

		double squaredDist_idx = RangingResults[idx].Distance * RangingResults[idx].Distance;
		double squaredDist_0 = RangingResults[0].Distance * RangingResults[0].Distance;

		b(idx - 1) = ApLocations[idx].SizeSquared() - ApLocations[0].SizeSquared() - squaredDist_idx + squaredDist_0;
	}

	Eigen::MatrixXd pusedoInverse = (A.transpose() * A).inverse();

	Eigen::Vector2d calpos = pusedoInverse * A.transpose() * b;

	FDVector2D NewPos(calpos(0), calpos(1));

	float Variance(0);
	for (int i = 0; i < RangingResults.Num(); ++i)
	{
		double DistanceNewPosToAP = FDVector2D(ApLocations[i] - NewPos).Size();
		double RangingDistance_i = RangingResults[i].Distance;
		double Deviation = DistanceNewPosToAP - RangingDistance_i;
		Variance += Deviation * Deviation;
	}
	Variance /= RangingResults.Num();

	if (OutDistribution)
	{
		*OutDistribution = Variance;
		return true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(120, 60, FColor::Cyan, FString::Printf(TEXT("Pos Var : %.2f"), Variance), true, FVector2D(3, 3));
	}

	if (Variance <= VALID_DISTRIBUTION)
	{
		if (bRecursive)
		{
			TArray<FRangingResult> RangingResultsLastCheck(RangingResults);
			for (const FString& filteredApID : FilteredAPs)
			{
				if (const FRangingResult* prevRangingResult = PrevRangingDataList.Find(filteredApID))
				{
					RangingResultsLastCheck.Add(*prevRangingResult);
				}
				else
				{
					return false;
				}
			}

			float resultVar = -1;
			if (DistributionFilterUpdate(RangingResultsLastCheck, &resultVar))
			{
				if (resultVar <= VALID_DISTRIBUTION)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	else if (RangingResults.Num() > 3)
	{
		float LowestVar(9000000.f);
		int32 FilterTargetIndex(-1);
		bool bHasFilterTargetIndex(false);

		for (int i = 0; i < RangingResults.Num(); i++)
		{
			TArray<FRangingResult> RangingResultsExceptSelf(RangingResults);
			RangingResultsExceptSelf.RemoveAt(i);

			float resultVar = -1;
			if (DistributionFilterUpdate(RangingResultsExceptSelf, &resultVar))
			{
				bHasFilterTargetIndex = true;
				if (resultVar < LowestVar)
				{
					LowestVar = resultVar;
					FilterTargetIndex = i;
				}
			}
		}
		if (bHasFilterTargetIndex)
		{
			GEngine->AddOnScreenDebugMessage(121, 60, FColor::Orange, FString::Printf(TEXT("Filtered Ap Data,  ID : %s, Distance : %.2f m"), *RangingResults[FilterTargetIndex].ApId, RangingResults[FilterTargetIndex].Distance), true, FVector2D(3, 3));

			FString FilterApId = RangingResults[FilterTargetIndex].ApId;

			FilteredAPs.AddUnique(FilterApId);
			RangingResults.RemoveAt(FilterTargetIndex);
			return DistributionFilterUpdate(RangingResults, nullptr, true);
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void AAnroidSensorRecorder::RecordRttRangingResults_Implementation(const FString& results)
{
#if PLATFORM_ANDROID
	if (UFusionPositioningSystemBPLibrary::bIsRecording == false)
	{
		UFusionPositioningSystemBPLibrary::bIsRecording = true;
		UFusionPositioningSystemBPLibrary::StartedRecordingTime = FPlatformTime::Seconds();
		UFusionPositioningSystemBPLibrary::RangingResultsForLogging.Empty(30000);
	}
	UE_LOG(LogTemp, Log, TEXT("#### RangingResult #### %s"), *results);

#endif

	bool bSuccess = false;
	int32 RequestCallbackID = -1;
	TArray<FRangingResult> rangingResults;
	currentTimestamp = -1;

	TArray<TSharedPtr<FJsonValue>> JsonArray;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(results);

	FString ReceivedAPList;
	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		auto Iter = JsonArray.CreateConstIterator();
		if (Iter)
		{
			const TSharedPtr<FJsonObject>& Header = (*Iter)->AsObject();

			bSuccess = Header->GetBoolField("Success");
			//RequestCallbackID = Header->GetIntegerField("RequestCallbackId");

			if (UFusionPositioningSystemBPLibrary::bReadyForManualRanging == false && UFusionPositioningSystemBPLibrary::bUseManualRanging && Header->GetBoolField("ManualRanging"))
			{
				UFusionPositioningSystemBPLibrary::bReadyForManualRanging = true;
				URTTDelegate::GetRTTDelegate()->ReadyForManualRangingDelegate.ExecuteIfBound();
			}
		}
		while (++Iter)
		{
			const TSharedPtr<FJsonObject>& obj = (*Iter)->AsObject();
			FRangingResult rangingResult;

			rangingResult.ApId = obj->GetStringField("ApID");
			rangingResult.Mac = obj->GetStringField("Mac");
			rangingResult.RSSI = obj->GetIntegerField("RSSI");
			rangingResult.Distance = obj->GetIntegerField("Distance") * 0.001f;
			rangingResult.DistanceStd = obj->GetIntegerField("DistanceStd");
			rangingResult.AttemptedMeasurements = obj->GetIntegerField("AttemptedMeasurements");
			rangingResult.SuccessfulMeasurements = obj->GetIntegerField("SuccessfulMeasurements");
			rangingResult.Timestamp = obj->GetStringField("Timestamp");
			rangingResult.LocX = obj->GetNumberField("LocX");
			rangingResult.LocY = obj->GetNumberField("LocY");
			rangingResult.LocZ = obj->GetNumberField("LocZ");

			int64 timestamp = FCString::Atoi64(*rangingResult.Timestamp);
			if (currentTimestamp < timestamp)
			{
				currentTimestamp = timestamp;
			}

			ReceivedAPList.Append(rangingResult.ApId + TEXT(" "));
			rangingResults.Add(rangingResult);			
		}
	}

	GEngine->AddOnScreenDebugMessage(105, 5, FColor::Green, FString::Printf(TEXT("RequestCallbackID : %d"), RequestCallbackID), true, FVector2D(2, 2));
	GEngine->AddOnScreenDebugMessage(106, 5, FColor::Green, FString::Printf(TEXT("RttAP Num : %d"), rangingResults.Num()), true, FVector2D(2, 2));
	GEngine->AddOnScreenDebugMessage(107, 5, FColor::Green, FString::Printf(TEXT("RttAPs : %s"), *ReceivedAPList), true, FVector2D(2, 2));


//#if PLATFORM_ANDROID
//	if (UFusionPositioningSystemBPLibrary::bIsRecording)
//	{
//		UFusionPositioningSystemBPLibrary::RangingResultsForLogging.Append(rangingResults);
//		int32 year, month, week, day, hour, min, sec, mSec;
//		FPlatformTime::SystemTime(year, month, week, day, hour, min, sec, mSec);
//		FString dateTime = FString::Printf(TEXT("%d-%2d-%2d %02d:%02d:%02d::%03d"), year, month, day, hour, min, sec, mSec);
//		if (FPlatformTime::Seconds() - UFusionPositioningSystemBPLibrary::StartedRecordingTime > 360)
//		{
//			UFusionPositioningSystemBPLibrary::SaveRttRangingLogData();
//			UFusionPositioningSystemBPLibrary::bIsRecording = false;
//		}
//	}
//#endif

	//if (UFusionPositioningSystemBPLibrary::bReadyForManualRanging)
	{
		//bSuccess = RttFilterUpdate(rangingResults);

		URTTDelegate::GetRTTDelegate()->OnRttManualRangingResultsRecieved.ExecuteIfBound(bSuccess, RequestCallbackID, rangingResults);
		/*if (bSuccess)
		{
			URTTDelegate::GetRTTDelegate()->OnRttRangingResultsRecieved.Broadcast(rangingResults);
		}*/
	}
	/*else if (UFusionPositioningSystemBPLibrary::bUseManualRanging == false)
	{
		if (rangingResults.Num() == 0)
		{
			return;
		}

		if (!RttFilterUpdate(rangingResults))
		{
			return;
		}

		UFusionPositioningSystemBPLibrary::CalculateRttLocation(rangingResults);
		URTTDelegate::GetRTTDelegate()->OnRttRangingResultsRecieved.Broadcast(rangingResults);
	}*/
}

void AAnroidSensorRecorder::RecordAccelometerEvent_Implementation(const FIMUSensor& Event)
{
	UAndroidServiceBPLibrary::GetAndroidServiceImpl()->OnAccSensorChangedDelegate.Broadcast(Event);
}

void AAnroidSensorRecorder::RecordGyroEvent_Implementation(const FIMUSensor& Event)
{
	UAndroidServiceBPLibrary::GetAndroidServiceImpl()->OnGyroSensorChangedDelegate.Broadcast(Event);
}

void AAnroidSensorRecorder::RecordGameRotationVectorEvent_Implementation(const FGameRotationVector& Event)
{
	UAndroidServiceBPLibrary::GetAndroidServiceImpl()->OnGameRotationVectorChangedDelegate.Broadcast(Event);
}