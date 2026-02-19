#include "Environment/LilypadActor.h"

ALilypadActor::ALilypadActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

}

void ALilypadActor::BeginPlay()
{
	Super::BeginPlay();
	
	HideElapsedTime = 0.0f;
	GetWorldTimerManager().SetTimer(HideTimerHandle, this, &ALilypadActor::UpdateLilypad, 0.1f, true, FMath::RandRange(0.0f, 4.0f));
}

void ALilypadActor::UpdateLilypad()
{
	HideElapsedTime += 0.1f;
	
	if (MeshComponent->IsVisible() == true)
	{
		if (HideElapsedTime >= 7.0f)
		{
			MeshComponent->SetVisibility(false);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HideElapsedTime = 0.0f;
		}
	}
	else
	{
		if (HideElapsedTime >= 2.0f)
		{
			MeshComponent->SetVisibility(true);
			MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
			HideElapsedTime = 0.0f;
		}
	}
}
