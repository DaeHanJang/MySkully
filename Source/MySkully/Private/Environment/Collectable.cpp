#include "Environment/Collectable.h"

#include "Components/SphereComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "Kismet/GameplayStatics.h"

ACollectable::ACollectable()
{
 	PrimaryActorTick.bCanEverTick = false;
	
	// 콜리전
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnCollectableBeginOverlap);
	CollisionComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 메시
	CollectableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollectableMesh"));
	CollectableMesh->SetupAttachment(GetRootComponent());
	CollectableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectableMesh->PrimaryComponentTick.bCanEverTick = false;
	
	// 점수
	Score = 1;
}

void ACollectable::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetWorldTimerManager().IsTimerActive(RotatorTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(RotatorTimerHandle, this, &ACollectable::UpdateRotation, 0.01f, true, 0.0f);
	}
}
void ACollectable::UpdateRotation()
{
	AddActorWorldRotation(FRotator(0.0f, 1.0f, 0.0f));
}

void ACollectable::OnCollectableBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == Player && OtherComp == Player->GetRootComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Collectable][OnCollectableBeginOverlap] OtherActor is Player And OtherComp is PlayerRootComponent"));
		ASkullyGameMode* GM = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Collectable][OnCollectableBeginOverlap] SkullyGameMode = nullptr"));
		}
		else
		{
			GM->AddScore(Score);
			UE_LOG(LogTemp, Warning, TEXT("[Collectable][OnCollectableBeginOverlap] Score: %d"), GM->GetScore());
		}
		RequestDestroy();
	}
}

void ACollectable::RequestDestroy()
{
	UE_LOG(LogTemp, Warning, TEXT("[Collectable][RequestDestroy]"));
	Destroy();
}
