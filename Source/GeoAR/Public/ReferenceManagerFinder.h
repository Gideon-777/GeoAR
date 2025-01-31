#pragma once

#include "CoreMinimal.h"
#include "ReferenceManagerFinder.generated.h"

UCLASS(BlueprintType)
class GEOAR_API UReferenceManager : public UObject
{
	GENERATED_UCLASS_BODY()
	public:
	UFUNCTION(BlueprintPure, Category = "ReferenceManager")
	static AActor* GetReferenceManager();

private:
	static TSubclassOf<class AActor> ReferenceManagerClass;
};