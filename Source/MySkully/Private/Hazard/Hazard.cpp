#include "Hazard/Hazard.h"

#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/SkullyCameraComponent.h"

AHazard::AHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	// 콜리전
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitBoxExtent(FVector(2500.0f, 2500.0f, 5000.0f));
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAll"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AHazard::OnBoxComponentBeginOverlap);
	CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AHazard::OnBoxComponentEndOverlap);
	CollisionComponent->PrimaryComponentTick.bCanEverTick = false;
	SetRootComponent(CollisionComponent);
	
	// 메시
	SurfaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SurfaceMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshAsset.Succeeded() == true)
	{
		SurfaceMesh->SetStaticMesh(PlaneMeshAsset.Object);
		SurfaceMesh->SetupAttachment(RootComponent);
		SurfaceMesh->SetRelativeLocation(FVector(0.0f, 0.0f, CollisionComponent->GetScaledBoxExtent().Z + 2.0f));
		SurfaceMesh->SetRelativeScale3D(FVector(50.0f, 50.0f, 10.0f));
		SurfaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SurfaceMesh->PrimaryComponentTick.bCanEverTick = false;
	}
	
	// 포스트 프로세스
	OverlapPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("OverlapPostProcess"));
	OverlapPostProcess->SetupAttachment(RootComponent);
	FPostProcessSettings& PS = OverlapPostProcess->Settings;
	PS.bOverride_ColorSaturation = true;
	PS.ColorSaturation = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
	PS.bOverride_ColorContrast = true;
	PS.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.0f);
	PS.bOverride_ColorGamma = true;
	PS.ColorGamma = FVector4(0.95f, 0.95f, 0.95f, 1.0f);
	PS.bOverride_ColorGain = true;
	PS.ColorGain = FVector4(0.5f, 0.7f, 1.0f, 1.0f);
	OverlapPostProcess->BlendWeight = 0.0f;
	OverlapPostProcess->PrimaryComponentTick.bCanEverTick = false;
}

void AHazard::OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][OnBoxComponentBeginOverlap] OtherActor = nullptr"));
		return;
	}
	
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][OnBoxComponentBeginOverlap] PlayerPawn = nullptr"));
		return;
	}
	
	if (OtherActor == Player)
	{
		const USkullyCameraComponent* PlayerCamera = Player->FindComponentByClass<USkullyCameraComponent>();
		
		// 플레이어 콜리전
		if (OtherComp == Player->GetRootComponent())
		{
			if (GetWorldTimerManager().IsTimerActive(DamageTimerHandle) == false)
			{
				GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &AHazard::DealDamage, DamageInterval, true, 0.0f);
			}
		}
		// 플레이어 카메라 콜리전
		else if (PlayerCamera != nullptr && OtherComp == PlayerCamera->GetCameraCollision())
		{
			OverlapPostProcess->BlendWeight = 1.0f;
		}		
	}
}

void AHazard::OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][OnBoxComponentEndOverlap] OtherActor = nullptr"));
		return;
	}
	
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][OnBoxComponentEndOverlap] PlayerPawn = nullptr"));
		return;
	}
	
	if (OtherActor == Player)
	{
		const USkullyCameraComponent* PlayerCamera = Player->FindComponentByClass<USkullyCameraComponent>();
		
		// 플레이어 콜리전
		if (OtherComp == Player->GetRootComponent())
		{
			GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		}
		// 플레이어 카메라 콜리전
		else if (PlayerCamera != nullptr && OtherComp == PlayerCamera->GetCameraCollision())
		{
			OverlapPostProcess->BlendWeight = 0.0f;
		}
	}
}

void AHazard::DealDamage() const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][DealDamage] PlayerController = nullptr"));
		return;
	}
	
	const APawn* Player = PC->GetPawn();
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][DealDamage] PlayerPawn = nullptr"));
		return;
	}
	
	UHealthComponent* HealthComponent = Player->FindComponentByClass<UHealthComponent>();
	if (HealthComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hazard.cpp][DealDamage] HealthComponent = nullptr"));
		return;
	}
	
	HealthComponent->LoseHealth(Damage);
}
