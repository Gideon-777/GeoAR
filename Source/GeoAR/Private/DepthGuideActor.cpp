// // Copyright 2017 Google Inc.//// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.

#include "DepthGuideActor.h"
#include "ARCameraManager.h"
#include <UMG/Public/Components/WidgetComponent.h>
#include <UMG/Public/Blueprint/UserWidget.h>
#include <UMG/Public/Components/CanvasPanel.h>
#include <UMG/Public/Components/Overlay.h>
#include <UMG/Public/Components/VerticalBox.h>
#include <UMG/Public/Components/HorizontalBox.h>
#include <UMG/Public/Components/VerticalBoxSlot.h>
#include <UMG/Public/Components/TextBlock.h>
#include <UMG/Public/Components/SlateWrapperTypes.h>
#include <Engine/UserInterfaceSettings.h>
#include <Geometry.h>
#include <Core/Public/Templates/SharedPointer.h>
#include <Engine/Public/Slate/SGameLayerManager.h>
#include <UMG/Public/Components/HorizontalBoxSlot.h>
#include <SlateCore/Public/Fonts/SlateFontInfo.h>

// Sets default values
ADepthGuideActor::ADepthGuideActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	PrevViewportSize(ForceInit)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	material = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("Material'/Game/Assets/Materials/Text3DWidgetMaterial.Text3DWidgetMaterial'")));

	static ConstructorHelpers::FClassFinder <UUserWidget> WidgetClass(TEXT("/Game/Assets/UI/DepthGuideX"));
	if (WidgetClass.Class != nullptr)
	{
		DepthGuideXClass = WidgetClass.Class;
	}
	static ConstructorHelpers::FClassFinder <UUserWidget> WidgetClass2(TEXT("/Game/Assets/UI/DepthInfoUI"));
	if (WidgetClass2.Class != nullptr)
	{
		DepthInfoUIClass = WidgetClass2.Class;
	}

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

FIntPoint ADepthGuideActor::GetCurrentViewportSize()
{
	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
	
	FIntPoint CurrentViewportSize(ForceInit);
	CurrentViewportSize.X = FGenericPlatformMath::FloorToInt(ViewportSize.X);
	CurrentViewportSize.Y = FGenericPlatformMath::FloorToInt(ViewportSize.Y);

	return CurrentViewportSize;
}

void ADepthGuideActor::UpdateDepthInfoUILayout()
{
	//for (int i = 0; i < GuideXWidgetList.Num(); i++)
	//{
	//	UTextBlock* UserWidget = *GuideInfoMap.Find(GuideXWidgetList[i]);

	//	UserWidget->ForceLayoutPrepass();

	//	FIntPoint CurrentViewportSize = GetCurrentViewportSize();
	//	float gui_scale_factor = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(CurrentViewportSize);
	//	FVector2D desiredSize = UserWidget->GetDesiredSize();
	//	desiredSize *= gui_scale_factor;

	//	FVector2D ViewportSize(CurrentViewportSize.X, CurrentViewportSize.Y);

	//	FVector2D newPosition;
	//	FVector2D screenCenterPos = ViewportSize * 0.5f - desiredSize * 0.5f;

	//	if (i % 2 == 0)
	//	{
	//		newPosition.X = screenCenterPos.X - desiredSize.X;
	//	}
	//	else
	//	{
	//		newPosition.X = screenCenterPos.X + desiredSize.X;
	//	}

	//	newPosition.Y = screenCenterPos.Y - (static_cast<float>(GuideXNumber - 1)) * (static_cast<float>(desiredSize.Y + DepthInfoUIPadding * gui_scale_factor)) * 0.5f + i * (static_cast<float>(desiredSize.Y + DepthInfoUIPadding * gui_scale_factor));

	//	//UserWidget->SetPositionInViewport(newPosition);
	//}
}

FVector2D ADepthGuideActor::GetWidgetPosition(UUserWidget* userWidgetObject)
{
	userWidgetObject->ForceLayoutPrepass();
	FVector2D AbsoluteCoordinate = userWidgetObject->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector);

	FVector2D ViewportPosition = FVector2D(0, 0);
	FVector2D PixelPosition = FVector2D(0, 0);
	if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
	{
		TSharedPtr<IGameLayerManager> GameLayerManager = ViewportClient->GetGameLayerManager();
		if (GameLayerManager.IsValid())
		{
			FVector2D ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);

			const FGeometry& ViewportGeometry = GameLayerManager->GetViewportWidgetHostGeometry();

			ViewportPosition = ViewportGeometry.AbsoluteToLocal(AbsoluteCoordinate);
			PixelPosition = (ViewportPosition / ViewportGeometry.GetLocalSize()) * ViewportSize;
		}
	}

	return PixelPosition;
}


void ADepthGuideActor::UpdateDepthInfoData()
{
	const float lineTranceLength = 1000000.0f; // 10 Km
	
	for (const TPair< UWidgetComponent*, UTextBlock* > pair : GuideInfoMap)
	{
		UWidgetComponent* guideX = pair.Key;
		UTextBlock* guideInfo = pair.Value;

		guideX->GetUserWidgetObject()->ForceLayoutPrepass();
		FIntPoint CurrentViewportSize = GetCurrentViewportSize();
		float gui_scale_factor = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(CurrentViewportSize);
		FVector2D desiredSize = guideX->GetUserWidgetObject()->GetDesiredSize();
		desiredSize *= gui_scale_factor;
		FVector2D ViewportSize(CurrentViewportSize.X, CurrentViewportSize.Y);
		FVector2D screenCenterPos = ViewportSize * 0.5f - desiredSize * 0.5f;
		FVector2D guidePosition = GetWidgetPosition(guideX->GetUserWidgetObject());
		guidePosition.X = screenCenterPos.X;
		guideX->GetUserWidgetObject()->SetPositionInViewport(guidePosition);
		
		
		
		FVector startLocation = guideX->GetComponentLocation();
		FVector endLocation = startLocation;
		endLocation.Z -= lineTranceLength;

		TArray<AActor*> ActorsToIgnore;

		FHitResult outHit;
		//bool const bHit = UKismetSystemLibrary::LineTraceSingle(this, startLocation, endLocation, ETraceTypeQuery::TraceTypeQuery4, false, ActorsToIgnore, EDrawDebugTrace::None, outHit, true, FLinearColor::Red, FLinearColor::Green, 60.0f);
		bool bHit = GetWorld()->LineTraceSingleByChannel(outHit, startLocation, endLocation, ECollisionChannel::ECC_GameTraceChannel1);
		//bool const bHit = GWorld->LineTraceSingleByChannel(OutHit, startLocation, endLocation, CollisionChannel, Params);

		if (bHit)
		{
			float depth = outHit.Location.Z;
			
			guideInfo->SetText(FText::FromString(FString::Printf(TEXT("%0.2f(m)"), depth * 0.01)));
		}
		else
		{
			bHit = GetWorld()->LineTraceSingleByChannel(outHit, startLocation, endLocation, ECollisionChannel::ECC_GameTraceChannel2);
			if (bHit)
			{
				float depth = outHit.Location.Z;

				guideInfo->SetText(FText::FromString(FString::Printf(TEXT("%0.2f(m)"), depth * 0.01)));
			}
			else
			{
				if (bHit)
				{
					guideInfo->SetText(FText::FromString(FString(TEXT("%0.0f(m)"))));
				}
			}
		}
	}
}

// Called when the game starts or when spawned
void ADepthGuideActor::BeginPlay()
{
	Super::BeginPlay();
	
	GuideXWidgetList.Empty();

	for (int i = 0; i < GuideXNumber; i++)
	{
		UWidgetComponent* newGuideX = NewObject<UWidgetComponent>(this);
		newGuideX->RegisterComponent();
		newGuideX->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		newGuideX->SetWidgetClass(DepthGuideXClass);
		newGuideX->SetRelativeLocation(FVector( (1+i) * GuideXIntervalMeter * 100.f , 0, 0));
		newGuideX->SetWidgetSpace(EWidgetSpace::Screen);
		newGuideX->SetMaterial(0, material);
		GuideXWidgetList.Add(newGuideX);
	}

	GuideInfoMap.Empty();
	/*for (int i = 0; i < GuideXNumber; i++)
	{
		UUserWidget* UserWidget = CreateWidget(GetWorld()->GetFirstPlayerController(), DepthInfoUIClass);
		UserWidget->AddToViewport();
		GuideInfoMap.Add(GuideXWidgetList[i], UserWidget);
	}*/

	UUserWidget* UserWidget = CreateWidget(GetWorld()->GetFirstPlayerController(), DepthInfoUIClass);
	UserWidget->AddToViewport();
	UVerticalBox* verticalBox = nullptr;

	FSlateColor DistanceTextColor(FLinearColor(0.955974f, 1.0f, 0.351533f));
	FSlateColor DepthTextColor(FLinearColor(0.291771f, 0.496933f, 1.0f));

	if (UCanvasPanel* canvasPanel = Cast<UCanvasPanel>(UserWidget->GetRootWidget()))
	{
		if (UOverlay* Overlay = Cast<UOverlay>(canvasPanel->GetChildAt(0)))
		{
			verticalBox = Cast<UVerticalBox>(Overlay->GetChildAt(1));
			if (verticalBox)
			{
				for (int i = GuideXNumber-1; i >= 0; i--)
				{
					UHorizontalBox* newHorizontalBox = NewObject<UHorizontalBox>(verticalBox);
					if (UVerticalBoxSlot* slot = Cast<UVerticalBoxSlot>(newHorizontalBox->Slot))
					{
						slot->SetHorizontalAlignment(HAlign_Fill);
						slot->SetVerticalAlignment(VAlign_Fill);
					}
					verticalBox->AddChildToVerticalBox(newHorizontalBox);

					UTextBlock* newTextBlock01 = NewObject<UTextBlock>(newHorizontalBox);
					newHorizontalBox->AddChildToHorizontalBox(newTextBlock01);

					UTextBlock* newTextBlock02 = NewObject<UTextBlock>(newHorizontalBox);
					newHorizontalBox->AddChildToHorizontalBox(newTextBlock02);
					if (UHorizontalBoxSlot* slot = Cast<UHorizontalBoxSlot>(newTextBlock02->Slot))
					{
						FSlateChildSize size;
						size.SizeRule = ESlateSizeRule::Fill;
						size.Value = 1.0;
						slot->SetSize(size);
						slot->SetHorizontalAlignment(HAlign_Right);
						slot->SetVerticalAlignment(VAlign_Center);
					}

					FSlateFontInfo font;
					font.FontObject = newTextBlock01->Font.FontObject;
					font.TypefaceFontName = TEXT("Bold");
					font.Size = 30;
					font.OutlineSettings.OutlineSize = 2;
					newTextBlock01->SetFont(font);
					newTextBlock02->SetFont(font);
					newTextBlock01->SetColorAndOpacity(DistanceTextColor);

					newTextBlock01->SetText(FText::FromString(FString::Printf(TEXT("%dM, "), static_cast<int>((i+1)*GuideXIntervalMeter))));
					newTextBlock02->SetText(FText::FromString(TEXT("0(m)")));
					newTextBlock02->SetColorAndOpacity(DepthTextColor);
					GuideInfoMap.Add(GuideXWidgetList[i], newTextBlock02);
				}
			}
		}
	}

	UpdateDepthInfoUILayout();
}

// Called every frame
void ADepthGuideActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FIntPoint CurrentViewportSize = GetCurrentViewportSize();
	if (PrevViewportSize != CurrentViewportSize)
	{
		UpdateDepthInfoUILayout();
		PrevViewportSize = CurrentViewportSize;
	}
	FRotator rotator(ForceInit);
	rotator.Yaw= GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation().Yaw;

	SetActorLocation(AARCameraManager::Get()->ArCoreTransform.GetLocation());
	SetActorRotation(rotator.Quaternion());

	UpdateDepthInfoData();
}

