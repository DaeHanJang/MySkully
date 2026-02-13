#include "GameFramework/SkullyPlayerController.h"

#include "IPlatformFilePak.h"
#include "Blueprint/UserWidget.h"
#include "SkullyHUDUserWidget.h"
#include "Environment/Collectable.h"
#include "GameFramework/SkullyGameMode.h"
#include "Kismet/GameplayStatics.h"

void ASkullyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] LocalController = false"));
		return;
	}
	if (HUDWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] HUDWidgetClass = nullptr"));
		return;
	}
	HUDWidgetInstance = CreateWidget<USkullyHUDUserWidget>(this, HUDWidgetClass);
	if (HUDWidgetInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] HUDWidgetInstance = nullptr"));
		return;
	}
	HUDWidgetInstance->AddToViewport(0);
	ASkullyGameMode* GM = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] SkullyGameMode = nullptr"));
		return;
	}
	GM->SetHUD(HUDWidgetInstance);
	
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACollectable::StaticClass(), Found);
	uint8 MaxScore = 0;
	for (const AActor* Actor : Found)
	{
		const ACollectable* Collectable = Cast<ACollectable>(Actor);
		if (Collectable != nullptr)
		{
			MaxScore += Collectable->GetScore();
		}
	}
	HUDWidgetInstance->SetCollectableMaxText(MaxScore);
}
