#include "Gollem/StrongGollemCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Hazard/Hazard.h"
#include "Skully/SkullyCameraComponent.h"

AStrongGollemCharacter::AStrongGollemCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 콜리전 설정
	RootComponent = GetCapsuleComponent();
	GetCapsuleComponent()->InitCapsuleSize(250.0f, 400.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);	
	
	// 메시 설정
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Character/Strong/Strong.Strong"));
	if (MeshAsset.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object, 0);
		GetMesh()->SetRelativeLocation(FVector(-100.0f, 0.0f, -390.0f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 상호작용 콜리전 생성
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(GetRootComponent());
	InteractionBox->InitBoxExtent(FVector(500.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// 스프링 암 생성
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 3000.0f;
	
	// 카메라 생성
	FollowCamera = CreateDefaultSubobject<USkullyCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	// 카메라 콜리전 생성
	CameraBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CameraBoxComponent"));
	CameraBoxComponent->SetupAttachment(FollowCamera);
	CameraBoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CameraBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CameraBoxComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	CameraBoxComponent->SetBoxExtent(FVector(60.0f), true);
	CameraBoxComponent->SetGenerateOverlapEvents(true);
	CameraBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AStrongGollemCharacter::OnCameraBoxComponentBeginOverlap);
	CameraBoxComponent->OnComponentEndOverlap.AddDynamic(this, &AStrongGollemCharacter::OnCameraBoxComponentEndOverlap);
	
	// 포스트 프로세싱 생성
	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcessComponent->SetupAttachment(FollowCamera);
	PostProcessComponent->BlendWeight = 0.0f;
	FPostProcessSettings& PS = PostProcessComponent->Settings;
	PS.bOverride_ColorSaturation = true;
	PS.ColorSaturation = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
	PS.bOverride_ColorContrast = true;
	PS.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.0f);
	PS.bOverride_ColorGamma = true;
	PS.ColorGamma = FVector4(0.95f, 0.95f, 0.95f, 1.0f);
	PS.bOverride_ColorGain = true;
	PS.ColorGain = FVector4(0.5f, 0.7f, 1.0f, 1.0f);
	
	// 헬스 컴포넌트 생성
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	
	// 캐릭터 설정
	bUseControllerRotationYaw = false;
	// 무브먼트 컴포넌트 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 1400.0f;
	GetCharacterMovement()->JumpZVelocity = 1500.0f;
}

void AStrongGollemCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (InteractionBox != nullptr)
	{
		InteractionBox->InitBoxExtent(FVector(500.0f));
	}
	
	if (CameraBoom != nullptr)
	{
		CameraBoom->TargetArmLength = 3000.0f;
	}
}

void AStrongGollemCharacter::OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	if (AHazard* Hazard = Cast<AHazard>(OtherActor))
	{
		PostProcessComponent->BlendWeight = 1.0f;
	}
}

void AStrongGollemCharacter::OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr)
	{
		return;
	}
	
	if (AHazard* Hazard = Cast<AHazard>(OtherActor))
	{
		PostProcessComponent->BlendWeight = 0.0f;
	}
}

void AStrongGollemCharacter::OnTakeDamage_Implementation()
{
}

void AStrongGollemCharacter::OnDeath_Implementation()
{
}

void AStrongGollemCharacter::OnTakeHealth_Implementation()
{
}

void AStrongGollemCharacter::InteractAction_Implementation()
{
	Super::InteractAction_Implementation();
	
	UE_LOG(LogTemp, Log, TEXT("[StrongGollem] InteractAction %s"), *GetName());
}

void AStrongGollemCharacter::PrimaryAction_Implementation()
{
	Super::PrimaryAction_Implementation();
	
	UE_LOG(LogTemp, Log, TEXT("[StrongGollem] PrimaryAction (Punch) %s"), *GetName());
}

void AStrongGollemCharacter::SecondaryAction_Implementation()
{
	Super::SecondaryAction_Implementation();
	
	UE_LOG(LogTemp, Log, TEXT("[StrongGollem] SecondaryAction (Slam) %s"), *GetName());
}
