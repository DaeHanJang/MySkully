#include "Hazard/Hazard.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/Character.h"
#include "Gollem/GollemCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

AHazard::AHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	// 콜라이더 생성
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	RootComponent = BoxComponent;
	BoxComponent->InitBoxExtent(FVector(2500.0f, 2500.0f, 5000.0f));
	BoxComponent->SetCollisionProfileName(TEXT("Hazard"));
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AHazard::OnBoxComponentBeginOverlap);
	BoxComponent->OnComponentEndOverlap.AddDynamic(this, &AHazard::OnBoxComponentEndOverlap);
	
	// Mesh
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshAsset.Succeeded() == true)
	{
		PlaneMesh->SetStaticMesh(PlaneMeshAsset.Object);
		PlaneMesh->SetupAttachment(BoxComponent);
		PlaneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoxComponent->GetScaledBoxExtent().Z + 2.0f));
		PlaneMesh->SetRelativeScale3D(FVector(50.0f, 50.0f, 1.0f));
		PlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
}

void AHazard::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHazard::OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	if (ASkully* Skully = Cast<ASkully>(OtherActor))
	{
		if (USphereComponent* SphereComponent = Cast<USphereComponent>(OtherComp))
		{
			if (GetWorldTimerManager().IsTimerActive(DamageTimerHandle) == false)
			{
				OverlappingSkully = Skully;
				GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &AHazard::DealDamageTick, DamageTickInterval, true, 0.0f);
			}
		}
	}
	else if (AGollemCharacter* Gollem = Cast<AGollemCharacter>(OtherActor))
	{
		if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(OtherComp))
		{
			if (GetWorldTimerManager().IsTimerActive(DamageTimerHandle) == false)
			{
				OverlappingGollem = Gollem;
				GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &AHazard::DealDamageTick, DamageTickInterval, true, 0.0f);
			}
		}
	}
}

void AHazard::OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr)
	{
		return;
	}
	
	if (ASkully* Skully = Cast<ASkully>(OtherActor))
	{
		if (USphereComponent* SphereComponent = Cast<USphereComponent>(OtherComp))
		{
			OverlappingSkully = nullptr;
			GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		}
	}
	else if (AGollemCharacter* Gollem = Cast<AGollemCharacter>(OtherActor))
	{
		if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(OtherComp))
		{
			OverlappingGollem = nullptr;
			GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		}
	}
}

void AHazard::DealDamageTick() const
{
	ASkully* Skully = OverlappingSkully.Get();
	AGollemCharacter* Gollem = OverlappingGollem.Get();
	
	if (Skully != nullptr)
	{
		if (UHealthComponent* HealthComponent = Skully->FindComponentByClass<UHealthComponent>())
		{
			HealthComponent->LoseHealth(DamagePerTick);
		}
	}
	else if (Gollem != nullptr)
	{
		if (UHealthComponent* HealthComponent = Gollem->FindComponentByClass<UHealthComponent>())
		{
			HealthComponent->LoseHealth(DamagePerTick);
		}
	}
}
