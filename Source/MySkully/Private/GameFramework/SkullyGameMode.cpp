#include "GameFramework/SkullyGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

ASkullyGameMode::ASkullyGameMode()
{
}

void ASkullyGameMode::RespawnPlayer()
{
	if (ASkully* Skully = Cast<ASkully>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (SaveIndex == 0)
		{
			SaveLocation = FindPlayerStart(GetWorld()->GetFirstPlayerController(), TEXT("Init"))->GetActorLocation() + FVector(0.0f, 0.0f, 350.0f);
		}
		Skully->SetActorLocation(SaveLocation);
		Skully->InitState();
	} 
}
