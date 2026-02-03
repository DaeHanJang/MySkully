#include "Environment/ClayMount.h"

#include "Components/BoxComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "Gollem/GollemCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

class ASkullyGameMode;

AClayMount::AClayMount()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메시 생성
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshAsset(TEXT("/Game/Environment/ClayMound/ClaySpawner.ClaySpawner"));
	if (StaticMeshAsset.Succeeded() == true)
	{
		StaticMesh->SetStaticMesh(StaticMeshAsset.Object);
		RootComponent = StaticMesh;
		StaticMesh->SetCollisionProfileName(TEXT("NoCollision"));
	}
	
	// 상호작용 감지 콜라이더 생성
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetBoxExtent(FVector(350.0f, 350.0f, 400.0f));
	BoxComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -350.0f));
	BoxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AClayMount::OnBoxComponentBeginOverlap);
	BoxComponent->OnComponentEndOverlap.AddDynamic(this, &AClayMount::OnBoxComponentEndOverlap);
	
	// 턱 콜라이더 생성
	const int32 NumSegments = 24;
	const float Radius = StaticMesh->Bounds.BoxExtent.X - 70.0f;
	for (int32 i = 0; i < NumSegments; i++)
	{
		float Angle = (2.0f * PI / NumSegments) * i;
		FVector Pos(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,-10.0f);

		UBoxComponent* Segment = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("Ring%d"), i));
		Segment->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		Segment->SetBoxExtent(FVector(50.0f, 50.0f, 10.0f));
		Segment->SetRelativeLocation(Pos);
		Segment->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
	
	// 바닥 콜라이더 생성
	UBoxComponent* Floor = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("Floor")));
	Floor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Floor->SetBoxExtent(FVector(400.0f, 400.0f, 30.0f));
	Floor->SetRelativeLocation(FVector(0.0f, 0.0f, -40.f));
	Floor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	// 세이브 인덱스 초기화
	SaveIndex = 0;
}

void AClayMount::BeginPlay()
{
	Super::BeginPlay();
	
}

void AClayMount::OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	if (ASkully* Skully = Cast<ASkully>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("On Clay Mound"));
		if (ASkullyGameMode* GameMode = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			if (SaveIndex > GameMode->GetSaveIndex())
			{
				GameMode->SetSaveLocation(GetActorLocation() + FVector(0.0f, 0.0f, 300.f));
				GameMode->SetSaveIndex(SaveIndex);
			}
		} 
		Skully->SetOnClayMound(true);
	}
	else if (AGollemCharacter* Gollem = Cast<AGollemCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("On Clay Mound"));
		if (ASkullyGameMode* GameMode = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			if (SaveIndex > GameMode->GetSaveIndex())
			{
				GameMode->SetSaveLocation(GetActorLocation() + FVector(0.0f, 0.0f, 300.f));
				GameMode->SetSaveIndex(SaveIndex);
			}
		}
		Gollem->SetOnClayMound(true);
	}
}

void AClayMount::OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr)
	{
		return;
	}
	
	if (ASkully* Skully = Cast<ASkully>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Off Clay Mound"));
		Skully->SetOnClayMound(false);
	}
	else if (AGollemCharacter* Gollem = Cast<AGollemCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Off Clay Mound"));
		Gollem->SetOnClayMound(false);
	}
}
