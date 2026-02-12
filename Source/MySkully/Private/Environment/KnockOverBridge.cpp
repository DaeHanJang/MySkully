#include "Environment/KnockOverBridge.h"

#include "Components/BoxComponent.h"

AKnockOverBridge::AKnockOverBridge()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 씬 컴포넌트(루트)
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
	SceneComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 메시
	BridgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BridgeMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BridgeMeshAsset(TEXT("/Game/Environment/KnockoverBridge/KnockoverBridge.KnockoverBridge"));
	if (BridgeMeshAsset.Succeeded() == true)
	{
		BridgeMesh->SetStaticMesh(BridgeMeshAsset.Object);
		BridgeMesh->SetupAttachment(GetRootComponent());
		BridgeMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
		BridgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BridgeMesh->PrimaryComponentTick.bCanEverTick = false;
	}
	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GateMeshAsset(TEXT("/Game/Environment/KnockoverBridge/GateKnockoverBridge/GateKnockOverBridge.GateKnockOverBridge"));
	if (GateMeshAsset.Succeeded() == true)
	{
		GateMesh->SetStaticMesh(GateMeshAsset.Object);
		GateMesh->SetupAttachment(GetRootComponent());
		GateMesh->SetRelativeLocation(FVector(220.0f, -160.0f, -2470.0f));
		GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GateMesh->PrimaryComponentTick.bCanEverTick = false;
	}
	
	// 블록 콜리전
	BridgeCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BridgeCollision"));
	BridgeCollision->SetupAttachment(BridgeMesh);
	BridgeCollision->InitBoxExtent(FVector(1900.0f, 650.0f, 200.0f));
	BridgeCollision->SetRelativeLocation(FVector(1970.0f, -140.0f, -70.0f));
	BridgeCollision->SetCollisionProfileName(TEXT("BlockAll"));
	BridgeCollision->PrimaryComponentTick.bCanEverTick = false;
	
	GateFloorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateFloorCollision"));
	GateFloorCollision->SetupAttachment(GateMesh);
	GateFloorCollision->InitBoxExtent(FVector(200.0f, 650.0f, 650.0f));
	GateFloorCollision->SetRelativeLocation(FVector(50.f, 20.0f, 1950.0f));
	GateFloorCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GateFloorCollision->PrimaryComponentTick.bCanEverTick = false;
	
	GateLeftCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateLeftCollision"));
	GateLeftCollision->SetupAttachment(GateMesh);
	GateLeftCollision->InitBoxExtent(FVector(500.0f, 200.0f, 500.0f));
	GateLeftCollision->SetRelativeLocationAndRotation(FVector(-343.0f, -950.0f, 2880.0f), FRotator(0.0f, 45.0f, 0.0f));
	GateLeftCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GateLeftCollision->PrimaryComponentTick.bCanEverTick = false;
	
	GateRightBottomCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateRightBottomCollision"));
	GateRightBottomCollision->SetupAttachment(GateMesh);
	GateRightBottomCollision->InitBoxExtent(FVector(1300.0f, 500.0f, 500.0f));
	GateRightBottomCollision->SetRelativeLocation(FVector(-265.0f, 1287.0f, 2270.0f));
	GateRightBottomCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GateRightBottomCollision->PrimaryComponentTick.bCanEverTick = false;
	
	GateRightTopCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateRightTopCollision"));
	GateRightTopCollision->SetupAttachment(GateMesh);
	GateRightTopCollision->InitBoxExtent(FVector(650.0f, 350.0f, 650.0f));
	GateRightTopCollision->SetRelativeLocationAndRotation(FVector(-743.0f, 1383.0f, 3340.0f), FRotator(0.0f, -10.0f, 0.0f));
	GateRightTopCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GateRightTopCollision->PrimaryComponentTick.bCanEverTick = false;
	
	// 감지 콜리전
	OverlapCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapCollision"));
	OverlapCollision->SetupAttachment(GetRootComponent());
	OverlapCollision->InitBoxExtent(FVector(300.0f, 650.0f, 650.0f));
	OverlapCollision->SetRelativeLocation(FVector(-140.0f, -140.0f, 600.0f));
	OverlapCollision->SetCollisionProfileName(TEXT("Overlap"));
	OverlapCollision->SetGenerateOverlapEvents(true);
	OverlapCollision->PrimaryComponentTick.bCanEverTick = false;
}

void AKnockOverBridge::KnockOver()
{
	KnockOverElapsed = 0.f;
	StartPitch = BridgeMesh->GetRelativeRotation().Pitch;
	
	if (GetWorldTimerManager().IsTimerActive(KnockOverTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(KnockOverTimerHandle, this, &AKnockOverBridge::UpdateKnockOverRotation, 0.01f, true, 0.0f);
	}
}

void AKnockOverBridge::UpdateKnockOverRotation()
{
	KnockOverElapsed += 0.01f;

	const float Alpha = FMath::Clamp(KnockOverElapsed / KnockOverDuration, 0.f, 1.f);
	const float Ease = FMath::Pow(Alpha, 4);
	const float NewPitch = FMath::Lerp(StartPitch, 0.f, Ease);

	FRotator R = BridgeMesh->GetRelativeRotation();
	R.Pitch = NewPitch;
	BridgeMesh->SetRelativeRotation(R);

	if (Alpha >= 1.f || NewPitch <= 0.f)
	{
		R.Pitch = 0.f;
		BridgeMesh->SetRelativeRotation(R);
		GetWorldTimerManager().ClearTimer(KnockOverTimerHandle);
	}
}
