#include "Skully/Skully.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Hazard/Hazard.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/SkullyCameraComponent.h"
#include "Skully/SkullyMovementComponent.h"

ASkully::ASkully()
{
 	PrimaryActorTick.bCanEverTick = false;
	
	// 콜리전 생성
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetSphereRadius(95.0f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SphereComponent->SetGenerateOverlapEvents(true);
	
	// 애로우 컴포넌트 생성
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->ArrowLength = 150.0f;
	
	// 메시 피벗 생성
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(RootComponent);
	MeshPivot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	
	// 스켈레탈 메시(Skully_Bone) 생성
	Skully_Bone = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skully_BoneMesh"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Skully_BoneMesh(TEXT("/Game/Character/Skully/Bone/Skully_Bone.Skully_Bone"));
	if (Skully_BoneMesh.Succeeded() == true)
	{
		Skully_Bone->SetSkeletalMesh(Skully_BoneMesh.Object);
		Skully_Bone->SetupAttachment(MeshPivot);
		Skully_Bone->SetRelativeLocation(FVector(10.0f, 0.0f, -8.0f));
		Skully_Bone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 스태틱 메시(Skully_Clay) 생성
	Skully_Clay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skully_ClayMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Skully_ClayMesh(TEXT("/Game/Character/Skully/Clay/Skully_Clay.Skully_Clay"));
	if (Skully_ClayMesh.Succeeded() == true)
	{
		Skully_Clay->SetStaticMesh(Skully_ClayMesh.Object);
		Skully_Clay->SetupAttachment(MeshPivot);
		Skully_Clay->SetRelativeLocation(FVector(20.0f, 0.0f, -8.0f));
		Skully_Clay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 스프링 암 생성
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));;
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->TargetArmLength = 1800.0f;
	CameraSpringArm->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));
	
	// 카메라 생성
	Camera = CreateDefaultSubobject<USkullyCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = true;
	
	// 카메라 콜리전 생성
	CameraBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CameraBoxComponent"));
	CameraBoxComponent->SetupAttachment(Camera);
	CameraBoxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CameraBoxComponent->SetGenerateOverlapEvents(true);
	CameraBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ASkully::OnCameraBoxComponentBeginOverlap);
	CameraBoxComponent->OnComponentEndOverlap.AddDynamic(this, &ASkully::OnCameraBoxComponentEndOverlap);
	
	// 포스트 프로세싱 생성
	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcessComponent->SetupAttachment(Camera);
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
	
	// 무브먼트 컴포넌트 생성
	SkullyMovementComponent = CreateDefaultSubobject<USkullyMovementComponent>(TEXT("MovementComponent"));
	SkullyMovementComponent->UpdatedComponent = SphereComponent;
	SkullyMovementComponent->VisualComponent = MeshPivot;
	
	// 헬스 컴포넌트 생성
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
			
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
	
	SkullyMovementComponent->OnMovementChanged.AddUObject(this, &ASkully::UpdateFOVBySpeed);
}

void ASkully::OnTakeDamage_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("HP: %f"), HealthComponent->GetHealth());
	SetSkully_ClayScale(HealthComponent->GetHealth() / 100.0f);
}

void ASkully::OnDeath_Implementation()
{
	if (ASkullyGameMode* GameMode = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->RespawnPlayer();
	}
}

void ASkully::OnTakeHealth_Implementation()
{
	if (bOnClayMound == false)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("HP: %f"), HealthComponent->GetHealth());
	SetSkully_ClayScale(HealthComponent->GetHealth() / 100.0f);
}

void ASkully::OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AHazard* Hazard = Cast<AHazard>(OtherActor))
	{
		if (UBoxComponent* BoxComponent = Cast<UBoxComponent>(OtherComp))
		{
			PostProcessComponent->BlendWeight = 1.0f;
		}
	}
}

void ASkully::OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AHazard* Hazard = Cast<AHazard>(OtherActor))
	{
		if (UBoxComponent* BoxComponent = Cast<UBoxComponent>(OtherComp))
		{
			PostProcessComponent->BlendWeight = 0.0f;
		}
	}
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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASkully::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASkully::StopJump);
		EnhancedInputComponent->BindAction(ClayMoundAction, ETriggerEvent::Started, this, &ASkully::Heal);
		EnhancedInputComponent->BindAction(ClayMoundAction, ETriggerEvent::Completed, this, &ASkully::StopHeal);
	}
}

// 속도 기반 FOV 갱신
void ASkully::UpdateFOVBySpeed(float DeltaTime, float Speed, FVector Dir)
{
	// 속도를 0~1 번위로 정규화
	const float Alpha = FMath::Clamp(Speed / SkullyMovementComponent->MaxSpeed, 0.0f, 1.0f);
	// 목표 FOV: 비율을 FOV 범위로 변환
	const float TargetFOV = FMath::Lerp(Camera->BaseFOV, Camera->MaxFOV, Alpha);
	// 현재 FOV를 목표 FOV로 부드럽게 변경
	const float NewFOV = FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaTime, Camera->FOVInterpSpeed);
	
	Camera->SetFieldOfView(NewFOV);
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

// 점프
void ASkully::Jump(const FInputActionValue& Value)
{
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->RequestJump();
	}
}
void ASkully::StopJump(const FInputActionValue& Value)
{
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->RequestJumpRelease();
	}
}

// 웅덩이 상호작용
void ASkully::Heal(const FInputActionValue& Value)
{
	if (bOnClayMound == false)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Healing Skully"));
	GetWorldTimerManager().SetTimer(HealTimerHandle, HealthComponent, &UHealthComponent::GainHealth, 0.02f, true, 0.0f);
}
void ASkully::StopHeal(const FInputActionValue& Value)
{	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
		UE_LOG(LogTemp, Warning, TEXT("End Healing Skully"));
	}
}

// 스컬리 초기화
void ASkully::InitState()
{
	HealthComponent->SetHealth(100.0f);
	SetSkully_ClayScale(HealthComponent->GetHealth() / 100.0f);
	SkullyMovementComponent->Velocity = FVector::ZeroVector;
}

// 스태틱 메시 스케일 설정
void ASkully::SetSkully_ClayScale(float Scale)
{
	Skully_Clay->SetWorldScale3D(FVector(Scale));
}
