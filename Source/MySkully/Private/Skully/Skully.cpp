#include "Skully/Skully.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SphereComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gollem/GollemCharacter.h"
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
		Skully_Bone->SetCollisionProfileName(TEXT("NoCollision"));
	}
	
	// 스태틱 메시(Skully_Clay) 생성
	Skully_Clay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skully_ClayMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Skully_ClayMesh(TEXT("/Game/Character/Skully/Clay/Skully_Clay.Skully_Clay"));
	if (Skully_ClayMesh.Succeeded() == true)
	{
		Skully_Clay->SetStaticMesh(Skully_ClayMesh.Object);
		Skully_Clay->SetupAttachment(MeshPivot);
		Skully_Clay->SetRelativeLocation(FVector(20.0f, 0.0f, -8.0f));
		Skully_Clay->SetCollisionProfileName(TEXT("NoCollision"));
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
	Camera->bUsePawnControlRotation = false;
	
	// 카메라 콜리전 생성
	CameraBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CameraBoxComponent"));
	CameraBoxComponent->SetupAttachment(Camera);
	CameraBoxComponent->SetCollisionProfileName(TEXT("SkullyCamera"));
	CameraBoxComponent->SetGenerateOverlapEvents(true);
	CameraBoxComponent->SetBoxExtent(FVector(60.0f), true);
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

void ASkully::SetNearbyGollem(AGollemCharacter* Gollem)
{
	NearbyGollem = Gollem;
}

void ASkully::ClearNearbyGollem(AGollemCharacter* Gollem)
{
	if (NearbyGollem.Get() == Gollem)
	{
		NearbyGollem = nullptr;
	}
}

void ASkully::BeginPlay()
{
	Super::BeginPlay();
	
	SkullyMovementComponent->OnMovementChanged.AddUObject(this, &ASkully::UpdateFOVBySpeed);
}

void ASkully::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void ASkully::UnPossessed()
{
	Super::UnPossessed();
}

void ASkully::PawnClientRestart()
{
	Super::PawnClientRestart();
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
	UE_LOG(LogTemp, Warning, TEXT("HP: %f"), HealthComponent->GetHealth());
	SetSkully_ClayScale(HealthComponent->GetHealth() / 100.0f);
}

void ASkully::OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

void ASkully::OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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
		EnhancedInputComponent->BindAction(ClayMoundAction, ETriggerEvent::Started, this, &ASkully::Interact);
		EnhancedInputComponent->BindAction(ClayMoundAction, ETriggerEvent::Completed, this, &ASkully::StopInteract);
		EnhancedInputComponent->BindAction(TransformStrongAction, ETriggerEvent::Triggered, this, &ASkully::TransformStrongGollem);
		EnhancedInputComponent->BindAction(TransformSwiftAction, ETriggerEvent::Triggered, this, &ASkully::TransformSwiftGollem);
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
	if (bTransitioningClayMound == true || bIsInClayMoundInteraction == true)
	{
		return;
	}
	
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
		
		if (CameraBoxComponent != nullptr)
		{
			CameraBoxComponent->UpdateOverlaps();
		}
	}
}

// 점프
void ASkully::Jump(const FInputActionValue& Value)
{
	if (bTransitioningClayMound == true || bIsInClayMoundInteraction == true)
	{
		return;
	}
	
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

// 상호작용
void ASkully::Interact(const FInputActionValue& Value)
{
	if (NearbyGollem.IsValid())
	{
		RideGollem(NearbyGollem.Get());
		return;
	}
	
	if (bOnClayMound == false)
	{
		return;
	}
	
	if (bTransitioningClayMound == true)
	{
		bIsInClayMoundInteraction = true;
		bClayMoundSubmerged = true;
		
		if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == false)
		{
			GetWorldTimerManager().SetTimer(HealTimerHandle, HealthComponent, &UHealthComponent::GainHealth, 0.02f, true, 0.0f);
		}
		
		return;
	}
	
	HideSkully(false);
	
	CameraSpringArm->ProbeChannel = ECC_GameTraceChannel1;
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->PlayerCameraManager->ViewPitchMin = -89.9f;
		PC->PlayerCameraManager->ViewPitchMax = -20.0f;
	}
	
	bIsInClayMoundInteraction = true;
	bTransitioningClayMound = true;
	bClayMoundSubmerged = true;
	
	if (bClayBaseLocked == false)
	{
		ClayMountSurfaceLocation = GetActorLocation();
		bClayBaseLocked = true;
	}
	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(HealTimerHandle, HealthComponent, &UHealthComponent::GainHealth, 0.02f, true, 0.0f);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(ClayTransitionTimerHandle, this, &ASkully::UpdateClayMoundTransition, 0.02f, true, 0.0f);
	}
}
void ASkully::StopInteract(const FInputActionValue& Value)
{	
	if (bClayBaseLocked == false && bTransitioningClayMound == false && bIsInClayMoundInteraction == false)
	{
		return;
	}
	
	bIsInClayMoundInteraction = false;
	bClayMoundSubmerged = false;
	bCanTransform = false;
	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle) == false)
	{
		bTransitioningClayMound = true;
		GetWorldTimerManager().SetTimer(ClayTransitionTimerHandle, this, &ASkully::UpdateClayMoundTransition, 0.02f, true, 0.0f);
	}
}

// 골렘 변신
void ASkully::TransformStrongGollem(const FInputActionValue& Value)
{
	if (bCanTransform == false)
	{
		return;
	}
	
	TransformToGollem(StrongGollemClass);
}
void ASkully::TransformSwiftGollem(const FInputActionValue& Value)
{
	if (bCanTransform == false)
	{
		return;
	}
	
	TransformToGollem(SwiftGollemClass);
}

// 웅덩이 연출
void ASkully::UpdateClayMoundTransition()
{
	const float Step = 0.02f / FMath::Max(0.25f, KINDA_SMALL_NUMBER);
	
	if (bClayMoundSubmerged == true)
	{
		ClayAlpha = FMath::Min(1.0f, ClayAlpha + Step);
	}
	else
	{
		ClayAlpha = FMath::Max(0.0f, ClayAlpha - Step);
	}
	
	const FVector TargetOffset(0.0f, 0.0f, -200.0f);
	const FVector NewLocation = FMath::Lerp(ClayMountSurfaceLocation, ClayMountSurfaceLocation + TargetOffset, ClayAlpha);
	
	SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	
	const bool bReached = (bClayMoundSubmerged == true && ClayAlpha >= 1.0f) || (bClayMoundSubmerged == false && ClayAlpha <= 0.0f);
	
	if (bReached == true)
	{
		bTransitioningClayMound = false;
		
		if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle) == true)
		{
			GetWorldTimerManager().ClearTimer(ClayTransitionTimerHandle);
		}
		
		// 웅덩이 끝까지 내려간 상태
		if (bClayMoundSubmerged == true)
		{
			bCanTransform = true;
		}
		// 웅덩이 끝까지 올라온 상태
		else
		{
			CameraSpringArm->ProbeChannel = ECC_Camera;
			
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				PC->PlayerCameraManager->ViewPitchMin = -89.9f;
				PC->PlayerCameraManager->ViewPitchMax = 89.9f;
			}
			
			ShowSkully();
			
			bClayBaseLocked = false;
		}
	}
}

// 변신
void ASkully::TransformToGollem(TSubclassOf<AGollemCharacter> GollemClass)
{
	if (GollemClass == nullptr)
	{
		return;
	}
	if (CurrentGollem != nullptr)
	{
		return;
	}
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}
		
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(ClayTransitionTimerHandle);
	}
	
	bIsInClayMoundInteraction = false;
	bTransitioningClayMound = false;
	bCanTransform = false;
	bClayMoundSubmerged = true;
	ClayAlpha = 1.0f;
	bClayBaseLocked = false;
	
	CameraSpringArm->ProbeChannel = ECC_Camera;
	
	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = 89.9f;
	
	HideSkully(true, true);
	
	FVector SpawnLocation = ClayMountSurfaceLocation;
	SpawnLocation.Z += 300.0f;
	
	FRotator SpawnRotation = FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f);
	
	FActorSpawnParameters Params;
	Params.Owner = PC;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AGollemCharacter* NewGollem = GetWorld()->SpawnActor<AGollemCharacter>(GollemClass, SpawnLocation, SpawnRotation, Params);
	if (NewGollem == nullptr)
	{
		return;
	}
	
	PC->Possess(NewGollem);
	CurrentGollem = NewGollem;
}

void ASkully::RideGollem(AGollemCharacter* Gollem)
{
	if (Gollem == nullptr)
	{
		return;
	}
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}
	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(ClayTransitionTimerHandle);
	}
	
	bIsInClayMoundInteraction = false;
	bTransitioningClayMound = false;
	bClayMoundSubmerged = false;
	bCanTransform = false;
	bClayBaseLocked = false;
	
	ClayAlpha = 0.0f;
	
	HideSkully(true, true);
	
	USkeletalMeshComponent* GollemMesh = Gollem->GetMesh();
	static const FName RideSocket(TEXT("L_DUMMY_JNT"));
	AttachToComponent(GollemMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RideSocket);
	
	const FRotator FaceRot(0.0f, Gollem->GetActorRotation().Yaw, 0.0f);
	PC->SetControlRotation(FaceRot);
	
	PC->Possess(Gollem);
	Gollem->SetInstigator(this);
}

// 스컬리 복귀
void ASkully::ReturnFromGollem(AGollemCharacter* FromGollem)
{
	if (FromGollem == nullptr)
	{
		return;
	}
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}
	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(ClayTransitionTimerHandle);
	}
	
	bIsInClayMoundInteraction = false;
	bTransitioningClayMound = false;
	bClayMoundSubmerged = false;
	bCanTransform = false;
	bClayBaseLocked = false;
	
	ClayAlpha = 0.0f;
	
	if (CameraSpringArm)
	{
		CameraSpringArm->ProbeChannel = ECC_Camera;
	}
	
	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = 89.9f;
	
	ShowSkully();
	
	FVector TargetLoc = FromGollem->GetActorLocation();
	TargetLoc.Z += 800.0f;
	SetActorLocation(TargetLoc, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f));
	
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->StopMovementImmediately();
		SkullyMovementComponent->RequestJump();
		SkullyMovementComponent->RequestJumpRelease();
	}
	PC->Possess(this);
	
	if (CurrentGollem == FromGollem)
	{
		CurrentGollem = nullptr;
	}
}

void ASkully::ReturnFromGollemDespawn(AGollemCharacter* FromGollem)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}
	
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	if (GetWorldTimerManager().IsTimerActive(ClayTransitionTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(ClayTransitionTimerHandle);
	}
	
	bIsInClayMoundInteraction = false;
	bTransitioningClayMound = false;
	bClayMoundSubmerged = false;
	bCanTransform = false;
	bClayBaseLocked = false;
	
	ClayAlpha = 0.0f;
	
	if (CameraSpringArm)
	{
		CameraSpringArm->ProbeChannel = ECC_Camera;
	}
	
	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = 89.9f;
	
	ShowSkully();
	
	FVector TargetLoc = FromGollem->GetActorLocation();
	TargetLoc.Z += 800.0f;
	SetActorLocation(TargetLoc, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f));
	
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->StopMovementImmediately();
	}
	PC->Possess(this);
	
	if (CurrentGollem == FromGollem)
	{
		CurrentGollem = nullptr;
	}
}

// 스컬리 초기화
void ASkully::InitState()
{
	HealthComponent->SetHealth(100.0f);
	SetSkully_ClayScale(HealthComponent->GetHealth() / 100.0f);
	SkullyMovementComponent->Velocity = FVector::ZeroVector;
}

// 스컬리 숨기기
void ASkully::HideSkully(bool bNoCollision, bool bMesh)
{
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->StopMovementImmediately();
		SkullyMovementComponent->SetComponentTickEnabled(false);
	}
	
	if (SphereComponent != nullptr)
	{
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		if (bNoCollision == true)
		{
			SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
		}
		else
		{
			SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		}
	}
	
	if (bMesh == true)
	{
		if (Skully_Bone != nullptr)
		{
			Skully_Bone->SetVisibility(false);
		}
		if (Skully_Clay != nullptr)
		{
			Skully_Clay->SetVisibility(false);
		}
	}
}

// 스컬리 보이기
void ASkully::ShowSkully()
{
	if (SphereComponent != nullptr)
	{
		SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
			
	if (SkullyMovementComponent != nullptr)
	{
		SkullyMovementComponent->SetComponentTickEnabled(true);
	}
	
	if (Skully_Bone != nullptr)
	{
		Skully_Bone->SetVisibility(true);
	}
	if (Skully_Clay != nullptr)
	{
		Skully_Clay->SetVisibility(true);
	}
}

// 스태틱 메시 스케일 설정
void ASkully::SetSkully_ClayScale(float Scale)
{
	Skully_Clay->SetWorldScale3D(FVector(Scale));
}
