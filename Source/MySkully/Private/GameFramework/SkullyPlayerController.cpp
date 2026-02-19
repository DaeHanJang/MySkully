#include "GameFramework/SkullyPlayerController.h"

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
	
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	if (LoadingWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] LoadingWidgetClass = nullptr"));
		return;
	}
	LoadingWidgetInstance = CreateWidget<UUserWidget>(this, LoadingWidgetClass);
	if (LoadingWidgetInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyPlayerController][BeginPlay] LoadingWidgetClass = nullptr"));
		return;
	}
	LoadingWidgetInstance->AddToViewport(100);
	FTimerHandle LoadingTimerHandle;
	GetWorldTimerManager().SetTimer(LoadingTimerHandle, this, &ASkullyPlayerController::ShowHUD, 8.0f, false);
}

void ASkullyPlayerController::ShowHUD()
{
	if (LoadingWidgetInstance)
	{
		LoadingWidgetInstance->RemoveFromParent();
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
	
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
}

void ASkullyPlayerController::RequestShowCheckPointUI()
{
	HUDWidgetInstance->ShowCheckPointUI();
}
