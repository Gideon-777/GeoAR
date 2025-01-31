#include "ReferenceManagerFinder.h"
#include "RmUtil.h"
#include <CoreUObject/Public/UObject/ConstructorHelpers.h>
#include <Kismet/GameplayStatics.h>

UReferenceManager::UReferenceManager(const FObjectInitializer& ObejctInitializer)
	: Super(ObejctInitializer)
{
	if (UReferenceManager::ReferenceManagerClass == nullptr)
	{
		static ConstructorHelpers::FClassFinder <AActor> ReferenceManager(TEXT("/Game/Blueprints/Manager/ReferenceManagerBP"));
		if (ReferenceManager.Class != nullptr)
		{
			UReferenceManager::ReferenceManagerClass = ReferenceManager.Class;
		}
	}
}
AActor* UReferenceManager::GetReferenceManager()
{
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorldStatic(), UReferenceManager::ReferenceManagerClass, foundActors);

	if (foundActors.Num() != 0)
	{
		return foundActors[0];
	}
	else
	{
		return Cast<AActor>(GetWorldStatic()->SpawnActor(UReferenceManager::ReferenceManagerClass));
	}
	return nullptr;
}

TSubclassOf<class AActor> UReferenceManager::ReferenceManagerClass = nullptr;