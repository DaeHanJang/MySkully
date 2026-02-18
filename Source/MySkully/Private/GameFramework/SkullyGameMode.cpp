#include "GameFramework/SkullyGameMode.h"

#include "SkullyHUDUserWidget.h"
#include "GameFramework/SkullyGameInstance.h"
#include "Golem/GolemCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Skully/Skully.h"

void ASkullyGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	auto* GI = GetGameInstance<USkullyGameInstance>();
	GI->PlayMusic("BGM", BGM);
	GI->PlayMusic("Bird", Bird, 0.3f);
	GI->PlayMusic("Wave", Wave, 0.5f);
}

void ASkullyGameMode::RespawnPlayer()
{
	++DeathCount;
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGolemCharacter::StaticClass(), Found);
	for (AActor* Actor : Found)
	{
		Actor->Destroy();
	}
	
	ASkully* Skully = Cast<ASkully>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Skully == nullptr)
	{
		return;
	}
	
	// 세이브 지점이 없을 때
	if (SaveIndex <= 0)
	{
		const FVector PlayerStartLocation = FindPlayerStart(UGameplayStatics::GetPlayerController(this, 0), TEXT("Init"))->GetActorLocation() + FVector(0.0f, 0.0f, 350.0f);
		Skully->SetActorLocationAndRotation(PlayerStartLocation, FRotator::ZeroRotator);
	}
	// 세이브 지점이 있을 때
	else
	{
		Skully->SetActorLocationAndRotation(SkullyRespawnLocation, FRotator::ZeroRotator);
	}
	Skully->Init();
}

void ASkullyGameMode::AddScore(const uint8 Value)
{
	Score += Value;
	HUD->UpdateCollectableText(Score);
}
