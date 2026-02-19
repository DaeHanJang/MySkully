#include "Environment/TargetAreaActor.h"

#include "SkullyHUDUserWidget.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "GameFramework/SkullyPlayerController.h"
#include "Kismet/GameplayStatics.h"

ATargetAreaActor::ATargetAreaActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TargetAreaComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TargetAreaComponent"));
	SetRootComponent(TargetAreaComponent);
	TargetAreaComponent->SetCollisionProfileName(TEXT("OverlapAll"));
	TargetAreaComponent->SetGenerateOverlapEvents(true);
	TargetAreaComponent->OnComponentBeginOverlap.AddDynamic(this, &ATargetAreaActor::OnTargetAreaComponentBeginOverlap);
	TargetAreaComponent->PrimaryComponentTick.bCanEverTick = false;
}

void ATargetAreaActor::OnTargetAreaComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == Player)
	{
		if (OtherComp == Player->GetRootComponent())
		{
			ASkullyGameMode* GM = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
			if (GM == nullptr)
			{
				return;
			}
			ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (PC == nullptr)
			{
				return;
			}
			PC->GetHUDWidget()->ShowResultUI(GM->GetScore(), GM->GetDeathCount());
		}
	}
}
