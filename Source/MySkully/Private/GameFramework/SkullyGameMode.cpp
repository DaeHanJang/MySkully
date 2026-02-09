#include "GameFramework/SkullyGameMode.h"

#include "GameFramework/SkullyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

ASkullyGameMode::ASkullyGameMode()
{
}

void ASkullyGameMode::RespawnPlayer()
{
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
