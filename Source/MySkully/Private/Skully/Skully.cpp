/*
* 파일명: Skully.cpp
* 생성일: 2026-01-08
* 수정일: 2026-01-09
* 내용: 플레이어 객체 Skully
*/

#include "Skully/Skully.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Skully/SkullyMovementComponent.h"

ASkully::ASkully()
{
 	PrimaryActorTick.bCanEverTick = false;
	
	// 콜라이더 생성
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetSphereRadius(95.0f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	// 애로우 컴포넌트 생성
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->ArrowLength = 150.0f;
	
	// 스켈레탈 메시(Skully_Bone) 생성
	Skully_Bone = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skully_BoneMesh"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Skully_BoneMesh(TEXT("/Game/Character/Skully/Bone/Skully_Bone.Skully_Bone"));
	if (Skully_BoneMesh.Succeeded() == true)
	{
		Skully_Bone->SetSkeletalMesh(Skully_BoneMesh.Object);
		Skully_Bone->SetupAttachment(RootComponent);
		Skully_Bone->SetRelativeLocation(FVector(10.0f, 0.0f, -8.0f));
	}
	
	// 스태틱 메시(Skully_Clay) 생성
	Skully_Clay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skully_ClayMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Skully_ClayMesh(TEXT("/Game/Character/Skully/Clay/Skully_Clay.Skully_Clay"));
	if (Skully_ClayMesh.Succeeded() == true)
	{
		Skully_Clay->SetStaticMesh(Skully_ClayMesh.Object);
		Skully_Clay->SetupAttachment(RootComponent);
		Skully_Clay->SetRelativeLocation(FVector(20.0f, 0.0f, -8.0f));
	}
	
	// 스프링 암 생성
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));;
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->TargetArmLength = 1800.0f;
	CameraSpringArm->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));
	
	// 카메라 생성
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = true;
	
	// 무브먼트 컴포넌트 생성
	MovementComponent = CreateDefaultSubobject<USkullyMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = SphereComponent;
		
	// 폰 설정
	// 컨트롤러 주입
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// 컨트롤러의 회전과 폰의 회전 동기화를 끔
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ASkully::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASkully::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASkully::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			SubSystem->AddMappingContext(InputMappingContext, 0);
		}
	}
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkully::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkully::Look);
	}
}

// 이동
void ASkully::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0, Rotation.Yaw, 0.0);
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

// 시점(카메라) 회전
void ASkully::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
