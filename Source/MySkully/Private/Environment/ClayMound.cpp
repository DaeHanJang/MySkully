#include "Environment/ClayMound.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "GameFramework/SkullyPlayerController.h"
#include "Kismet/GameplayStatics.h"

AClayMound::AClayMound()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	// 메시
	ClayMoundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClayMoundMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ClayMoundMeshAsset(TEXT("/Game/Environment/ClayMound/ClaySpawner.ClaySpawner"));
	if (ClayMoundMeshAsset.Succeeded() == true)
	{
		ClayMoundMesh->SetStaticMesh(ClayMoundMeshAsset.Object);
		ClayMoundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ClayMoundMesh->PrimaryComponentTick.bCanEverTick = false;
		SetRootComponent(ClayMoundMesh);
	}
	
	// 상호작용 콜리전
	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(RootComponent);
	InteractionCollision->SetBoxExtent(FVector(300.0f, 300.0f, 400.0f));
	InteractionCollision->SetRelativeLocation(FVector(0.0f, 0.0f, -350.0f));
	InteractionCollision->SetCollisionProfileName(TEXT("OverlapAll"));
	InteractionCollision->SetGenerateOverlapEvents(true);
	InteractionCollision->OnComponentBeginOverlap.AddDynamic(this, &AClayMound::OnBoxComponentBeginOverlap);
	InteractionCollision->OnComponentEndOverlap.AddDynamic(this, &AClayMound::OnBoxComponentEndOverlap);
	InteractionCollision->PrimaryComponentTick.bCanEverTick = false;
	
	// 블록 콜리전 피벗
	BlockerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BlockerPivot"));
	BlockerPivot->SetupAttachment(RootComponent);
	BlockerPivot->PrimaryComponentTick.bCanEverTick = false;
	
	// 바닥 콜리전
	UBoxComponent* Floor = CreateDefaultSubobject<UBoxComponent>(TEXT("Floor"));
	Floor->SetupAttachment(BlockerPivot);
	Floor->SetBoxExtent(FVector(400.0f, 400.0f, 30.0f));
	Floor->SetRelativeLocation(FVector(0.0f, 0.0f, -40.f));
	Floor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Floor->PrimaryComponentTick.bCanEverTick = false;
	// 턱 콜리전
	const uint8 NumSegments = 16;
	const float Radius = ClayMoundMesh->Bounds.BoxExtent.X - 70.0f;
	for (uint8 i = 0; i < NumSegments; ++i)
	{
		const float Angle = (2.0f * PI / NumSegments) * i;
		const FVector Pos(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,-10.0f);
		const FName SegmentName = *FString::Printf(TEXT("Ring%d"), i);
		
		UBoxComponent* Segment = CreateDefaultSubobject<UBoxComponent>(SegmentName);
		Segment->SetupAttachment(BlockerPivot);;
		Segment->SetBoxExtent(FVector(50.0f, 50.0f, 10.0f));
		Segment->SetRelativeLocation(Pos);
		Segment->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		Segment->PrimaryComponentTick.bCanEverTick = false;
	}
	
	// 사운드
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);
	AudioComp->bAutoActivate = false;
}

void AClayMound::BeginPlay()
{
	Super::BeginPlay();
	
	AudioComp->SetSound(LoopSound);
	AudioComp->Play();
}

void AClayMound::OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentBeginOverlap] PlayerPawn = nullptr"));
		return;
	}
	
	if (OtherActor == Player)
	{
		if (OtherComp == Player->GetRootComponent())
		{
			UClayMoundReactiveComponent* ClayMoundReactiveComponent = Player->FindComponentByClass<UClayMoundReactiveComponent>();
			if (ClayMoundReactiveComponent == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentBeginOverlap] ClayMoundReactiveComponent = nullptr"));
				return;
			}
			
			UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentBeginOverlap]"));
			ClayMoundReactiveComponent->SetOnClayMoundSurface(true, BlockerPivot->GetComponentLocation());
			
			ASkullyGameMode* GM = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
			if (GM == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentBeginOverlap] SkullyGameMode = nullptr"));
				return;
			}
			if (SavePriority > GM->GetSaveIndex())
			{
				GM->SetSkullyRespawnLocation(GetActorLocation());
				GM->SetSaveIndex(SavePriority);
				ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
				if (PC == nullptr)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentBeginOverlap] SkullyPlayerController = nullptr"));
					return;
				}
				PC->RequestShowCheckPointUI();
			}
		}
	}
}

void AClayMound::OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr)
	{
		return;
	}
	
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentEndOverlap] PlayerPawn = nullptr"));
		return;
	}
	
	if (OtherActor == Player)
	{
		if (OtherComp == Player->GetRootComponent())
		{
			UClayMoundReactiveComponent* ClayMoundReactiveComponent = Player->FindComponentByClass<UClayMoundReactiveComponent>();
			if (ClayMoundReactiveComponent == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentEndOverlap] ClayMoundReactiveComponent = nullptr"));
				return;
			}
			
			UE_LOG(LogTemp, Warning, TEXT("[ClayMound.cpp][OnBoxComponentEndOverlap]"));
			ClayMoundReactiveComponent->SetOnClayMoundSurface(false, FVector::ZeroVector);
		}
	}
}
