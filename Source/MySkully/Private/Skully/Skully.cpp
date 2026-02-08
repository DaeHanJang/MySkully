#include "Skully/Skully.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/SkullyGameMode.h"
#include "GameFramework/SkullyPlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Golem/GolemCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/SkullyCameraComponent.h"
#include "Skully/SkullyMovementComponent.h"

ASkully::ASkully()
{
 	PrimaryActorTick.bCanEverTick = false;
	
	// 콜리전
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(95.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->PrimaryComponentTick.bCanEverTick = false;
	SetRootComponent(CollisionComponent);
	
	// 애로우 컴포넌트
	Direction = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction"));
	Direction->SetupAttachment(RootComponent);
	Direction->SetArrowLength(150.0f);
	Direction->PrimaryComponentTick.bCanEverTick = false;
	
	// 메시 피벗
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(RootComponent);
	MeshPivot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	MeshPivot->PrimaryComponentTick.bCanEverTick = false;
	
	// 스켈레탈 메시
	BoneMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BoneMesh"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BoneMeshAsset(TEXT("/Game/Character/Skully/Bone/Skully_Bone.Skully_Bone"));
	if (BoneMeshAsset.Succeeded() == true)
	{
		BoneMesh->SetSkeletalMesh(BoneMeshAsset.Object);
		BoneMesh->SetupAttachment(MeshPivot);
		BoneMesh->SetRelativeLocation(FVector(10.0f, 0.0f, -8.0f));
		BoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 스태틱 메시
	ClayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClayMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ClayMeshAsset(TEXT("/Game/Character/Skully/Clay/Skully_Clay.Skully_Clay"));
	if (ClayMeshAsset.Succeeded() == true)
	{
		ClayMesh->SetStaticMesh(ClayMeshAsset.Object);
		ClayMesh->SetupAttachment(MeshPivot);
		ClayMesh->SetRelativeLocation(FVector(20.0f, 0.0f, -8.0f));
		ClayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ClayMesh->PrimaryComponentTick.bCanEverTick = false;
	}
	
	// 스프링 암
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));;
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 1800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));
	
	// 카메라
	FollowCamera = CreateDefaultSubobject<USkullyCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->PrimaryComponentTick.bCanEverTick = false;
	
	// 카메라 콜리전
	FollowCamera->SetCameraCollision(CreateDefaultSubobject<UBoxComponent>(TEXT("FollowCameraCollision")));
	FollowCameraCollision = FollowCamera->GetCameraCollision();
	FollowCameraCollision->SetupAttachment(FollowCamera);
	FollowCameraCollision->InitBoxExtent(FVector(60.0f));
	FollowCameraCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	FollowCameraCollision->SetGenerateOverlapEvents(true);
	FollowCameraCollision->PrimaryComponentTick.bCanEverTick = false;
	
	// 무브먼트 컴포넌트
	SkullyMovementComponent = CreateDefaultSubobject<USkullyMovementComponent>(TEXT("SkullyMovementComponent"));
	SkullyMovementComponent->SetUpdatedComponent(GetRootComponent());
	SkullyMovementComponent->SetPivot(MeshPivot);
	
	// 헬스 컴포넌트
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 웅덩이 상호작용 컴포넌트
	ClayMoundReactiveComponent = CreateDefaultSubobject<UClayMoundReactiveComponent>(TEXT("ClayMoundReactiveComponent"));
	ClayMoundReactiveComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 폰 설정
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ASkully::BeginPlay()
{
	Super::BeginPlay();
	
	ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][BeginPlay] SkullyPlayerController = nullptr"));
	}
	else
	{
		PC->SetSkully(this);
	}
	
	if (SkullyMovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][BeginPlay] SkullyMovementComponent = nullptr"));
		return;
	}
	// 속도 기반 카메라 FOV 갱신
	SkullyMovementComponent->OnMovementChanged.AddUObject(this, &ASkully::UpdateCameraFOVFromSpeed);
}

void ASkully::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][PossessedBy] PlayerController = nullptr"));
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][PossessedBy] LocalPlayer = nullptr"));
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][PossessedBy] EnhancedInputLocalPlayerSubsystem = nullptr"));
		return;
	}
	
	// IMC 연결
	EILPS->AddMappingContext(InputMappingContext, 0);
}

void ASkully::UnPossessed()
{
	UE_LOG(LogTemp, Warning, TEXT("Skully UnPossessed"));
	
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][UnPossessed] PlayerController = nullptr"));
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][UnPossessed] LocalPlayer = nullptr"));
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][UnPossessed] EnhancedInputLocalPlayerSubsystem = nullptr"));
		return;
	}
	
	// IMC 해제
	EILPS->RemoveMappingContext(InputMappingContext);
	
	Super::UnPossessed();
}

void ASkully::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] EnhancedInputComponent = nullptr"));
		return;
	}
	
	// 이동 맵핑
	if (MoveInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] MoveInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &ASkully::Move);
	}
	// 시점 맵핑
	if (LookInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] LookInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ASkully::Look);
	}
	// 점프 맵핑
	if (JumpInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] JumpInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(JumpInputAction, ETriggerEvent::Started, this, &ASkully::Jump);
		EIC->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ASkully::StopJump);
	}
	// 웅덩이 상호작용 맵핑
	if (ClayMoundInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] ClayMoundInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(ClayMoundInputAction, ETriggerEvent::Started, this, &ASkully::Interact);
		EIC->BindAction(ClayMoundInputAction, ETriggerEvent::Completed, this, &ASkully::StopInteract);
	}
	// 스트롱 골렘 변신 맵핑
	if (TransformStrongGolemInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] TransformStrongGolemInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(TransformStrongGolemInputAction, ETriggerEvent::Triggered, this, &ASkully::TransformStrongGolem);
	}
	// 스위프트 골렘 변신 맵핑
	if (TransformSwiftGolemInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] TransformSwiftGolemInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(TransformSwiftGolemInputAction, ETriggerEvent::Triggered, this, &ASkully::TransformSwiftGolem);
	}
	// 골렘 상호작용 맵핑
	if (GolemInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][SetupPlayerInputComponent] RideInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(GolemInputAction, ETriggerEvent::Triggered, this, &ASkully::GolemInteract);
	}
}

void ASkully::OnTakeDamage_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnTakeDamage_Implementation] HP: %f"), HealthComponent->GetHealth());
	// 체력 비례 ClayMesh 스케일 갱신 
	ClayMesh->SetRelativeScale3D(FVector(HealthComponent->GetHealth() / 100.0f));
}
void ASkully::OnDeath_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnDeath_Implementation] HP: %f"), HealthComponent->GetHealth());
	
	ASkullyGameMode* GM = Cast<ASkullyGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnDeath_Implementation] SkullyGameMode = nullptr"));
		return;
	}
	
	GM->RespawnPlayer();
}
void ASkully::OnTakeHealth_Implementation()
{	
	UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnTakeHealth_Implementation] HP: %f"), HealthComponent->GetHealth());
	// 체력 비례 ClayMesh 스케일 갱신
	ClayMesh->SetRelativeScale3D(FVector(HealthComponent->GetHealth() / 100.0f));
}

void ASkully::OnEnterClayMound_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnEnterClayMound_Implementation] EnterClayMound"));
	bClayMoundInteraction = false;
	ClayMoundTransitionAlpha = 0.0f;
}
void ASkully::OnExitClayMound_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][OnExitClayMound_Implementation] ExitClayMound"));
}

void ASkully::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Move] Controller = nullptr"));
		return;
	}
	
	const FVector2D MovementVector2D = Value.Get<FVector2D>();
	
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0, Rotation.Yaw, 0.0);
		
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
	AddMovementInput(ForwardDirection, MovementVector2D.Y);
	AddMovementInput(RightDirection, MovementVector2D.X);
}

void ASkully::Look(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Look] Controller = nullptr"));
		return;
	}
	
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
	
	if (FollowCameraCollision == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Look] FollowCameraCollision = nullptr"));
	}
	else
	{
		FollowCameraCollision->UpdateOverlaps();
	}
}

void ASkully::Jump(const FInputActionValue& Value)
{
	if (bClayMoundInteraction == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Jump] bClayMoundInteraction = true"));
		return;
	}
	
	if (SkullyMovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Jump] SkullyMovementComponent = nullptr"));
		return;
	}
	SkullyMovementComponent->RequestJump();
}
void ASkully::StopJump(const FInputActionValue& Value)
{
	if (SkullyMovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][StopJump] SkullyMovementComponent = nullptr"));
		return;
	}
	SkullyMovementComponent->RequestJumpRelease();
}

void ASkully::Interact(const FInputActionValue& Value)
{
	if (ClayMoundReactiveComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Interact] ClayMoundReactiveComponent = nullptr"));
		return;
	}
	if (ClayMoundReactiveComponent->GetOnClayMoundSurface() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][Interact] bOnClayMound = false"));
		return;
	}
	
	// 체력 회복
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(HealTimerHandle, HealthComponent.Get(), &UHealthComponent::GainHealth, 0.02f, true, 0.0f);
	}
	
	// 잠수 연출
	HideSkully(false, true);
	CameraBoom->ProbeChannel = ECC_GameTraceChannel1;
	const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = -20.0f;
	
	bClayMoundInteraction = true;
	bDescendingIntoClayMound = true;
	if (GetWorldTimerManager().IsTimerActive(ClayMoundTransitionTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(ClayMoundTransitionTimerHandle, this, &ASkully::UpdateClayMoundTransition, 0.02f, true, 0.0f);
	}
}
void ASkully::StopInteract(const FInputActionValue& Value)
{
	if (ClayMoundReactiveComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][StopInteract] ClayMoundReactiveComponent = nullptr"));
		return;
	}
	if (ClayMoundReactiveComponent->GetOnClayMoundSurface() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][StopInteract] bOnClayMound = false"));
		return;
	}
	
	// 체력 회복 중단
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	
	// 부상 연출
	bCanTransform = false;
	bDescendingIntoClayMound = false;
	if (GetWorldTimerManager().IsTimerActive(ClayMoundTransitionTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(ClayMoundTransitionTimerHandle, this, &ASkully::UpdateClayMoundTransition, 0.02f, true, 0.0f);
	}
}
void ASkully::UpdateClayMoundTransition()
{
	const float Step = 0.02f / FMath::Max(0.25f, KINDA_SMALL_NUMBER);
	
	if (bDescendingIntoClayMound == true)
	{
		ClayMoundTransitionAlpha = FMath::Min(1.0f, ClayMoundTransitionAlpha + Step);
	}
	else
	{
		ClayMoundTransitionAlpha = FMath::Max(0.0f, ClayMoundTransitionAlpha - Step);
	}
	
	const FVector TargetLocation = ClayMoundReactiveComponent->GetClayMoundSurfaceLocation() + FVector(0.0f, 0.0f, -200.0f);
	const FVector NewLocation = FMath::Lerp(ClayMoundReactiveComponent->GetClayMoundSurfaceLocation(), TargetLocation, ClayMoundTransitionAlpha);
	SetActorLocation(NewLocation);
	
	const bool bReached = (bDescendingIntoClayMound == true && ClayMoundTransitionAlpha >= 1.0f) || (bDescendingIntoClayMound == false && ClayMoundTransitionAlpha <= 0.0f);
	if (bReached == true)
	{		
		if (GetWorldTimerManager().IsTimerActive(ClayMoundTransitionTimerHandle) == true)
		{
			GetWorldTimerManager().ClearTimer(ClayMoundTransitionTimerHandle);
		}
		
		// 웅덩이 끝까지 잠수
		if (bDescendingIntoClayMound == true)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][UpdateClayMoundTransition] IsSubmerged"));
			bCanTransform = true;
		}
		// 웅덩이 표면에 도달
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][UpdateClayMoundTransition] IsSurface"));
			ShowSkully();
			CameraBoom->ProbeChannel = ECC_Camera;
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			PC->PlayerCameraManager->ViewPitchMin = -89.9f;
			PC->PlayerCameraManager->ViewPitchMax = 89.9f;
			bClayMoundInteraction = false;
		}
	}
}

void ASkully::TransformStrongGolem(const FInputActionValue& Value)
{
	if (bCanTransform == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][TransformStrongGolem] bCanTransform = false"));
		return;
	}
	
	TransformToGolem(StrongGolemClass, 450.0f);
}
void ASkully::TransformSwiftGolem(const FInputActionValue& Value)
{
	if (bCanTransform == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][TransformSwiftGolem] bCanTransform = false"));
		return;
	}
	
	TransformToGolem(SwiftGolemClass, 400.0f);
}
void ASkully::TransformToGolem(const TSubclassOf<AGolemCharacter> GolemClass, const float ZOffset)
{
	if (GolemClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][TransformToGolem] GolemClass = nullptr"));
		return;
	}
	if (CurrentGolem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][TransformToGolem] CurrentGolem != nullptr"));
		return;
	}
	
	bCanTransform = false;
	bClayMoundInteraction = false;
	bDescendingIntoClayMound = false;
	ClayMoundTransitionAlpha = 0.0f;
	if (GetWorldTimerManager().IsTimerActive(HealTimerHandle) == true)
	{
		GetWorldTimerManager().ClearTimer(HealTimerHandle);
	}
	ClayMoundReactiveComponent->SetOnClayMoundSurface(false, FVector::ZeroVector);
	CameraBoom->ProbeChannel = ECC_Camera;
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = 89.9f;
	
	FVector SpawnLocation = ClayMoundReactiveComponent->GetClayMoundSurfaceLocation();
	SpawnLocation.Z += ZOffset;
	const FRotator SpawnRotation = FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f);
	FActorSpawnParameters Params;
	Params.Owner = PC;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AGolemCharacter* NewGolem = GetWorld()->SpawnActor<AGolemCharacter>(GolemClass, SpawnLocation, SpawnRotation, Params);
	if (NewGolem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][TransformToGolem] NewGolem = nullptr"));
		return;
	}
	
	PC->Possess(NewGolem);
	CurrentGolem = NewGolem;
}

void ASkully::GolemInteract(const FInputActionValue& Value)
{
	if (bCanRide == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][GolemInteract] bCanRide = false"));
		return;
	}
	if (NearbyGolem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][GolemInteract] NearbyGolem == nullptr"));
		return;
	}
	if (bClayMoundInteraction == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][GolemInteract] bClayMoundInteraction = true"));
		return;
	}
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	HideSkully(false, true);
	
	PC->Possess(NearbyGolem);
	CurrentGolem = NearbyGolem;
}

void ASkully::UpdateCameraFOVFromSpeed(float DeltaTime, float Speed, FVector Dir) const
{
	// 속도 정규화
	const float Alpha = FMath::Clamp(Speed / SkullyMovementComponent->GetMaxSpeed(), 0.0f, 1.0f);
	// 목표 FOV
	const float TargetFOV = FMath::Lerp(DefaultFOV, MaxFOV, Alpha);
	// 현재 FOV->목표 FOV 보간
	const float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, FOVInterpSpeed);	
	FollowCamera->SetFieldOfView(NewFOV);
}

void ASkully::DismountGolem(const float ZOffset)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}
	
	FVector TargetLoc = CurrentGolem->GetActorLocation();
	TargetLoc.Z += ZOffset;
	SetActorLocationAndRotation(TargetLoc, FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	
	ShowSkully();
	
	if (SkullyMovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][ReturnFromGolem] SkullyMovementComponent = nullptr"));
	}
	else
	{
		SkullyMovementComponent->StopMovementImmediately();
		SkullyMovementComponent->RequestJump();
		SkullyMovementComponent->RequestJumpRelease();
	}
	
	PC->Possess(this);
	CurrentGolem = nullptr;
}

void ASkully::DespawnGolem()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][BegineDespawnGolem] PlayerController = nullptr"));
		return;
	}
	
	SetActorLocationAndRotation(CurrentGolem->GetClayMoundSurfaceLocation(), FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	
	ShowSkully();
	
	PC->Possess(this);
	CurrentGolem = nullptr;
}

void ASkully::Init() const
{
	HealthComponent->SetHealth(100.0f);
	ClayMesh->SetRelativeScale3D(FVector(HealthComponent->GetHealth() / 100.0f));
	SkullyMovementComponent->Velocity = FVector::ZeroVector;
}

void ASkully::HideSkully(const bool bNoCollision, const bool bMesh) const
{
	if (SkullyMovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][HideSkully] SkullyMovementComponent = nullptr"));
	}
	else
	{
		SkullyMovementComponent->StopMovementImmediately();
		SkullyMovementComponent->SetComponentTickEnabled(false);
	}
	
	if (CollisionComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][HideSkully] CollisionComponent = nullptr"));
	}
	else
	{
		if (bNoCollision == true)
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else
		{
			CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		}
	}
	
	if (BoneMesh == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][HideSkully] BoneMesh = nullptr"));
	}
	else
	{
		BoneMesh->SetVisibility(bMesh);
	}
	if (ClayMesh == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][HideSkully] ClayMesh = nullptr"));
	}
	else
	{
		ClayMesh->SetVisibility(bMesh);
	}
}

void ASkully::ShowSkully(const bool bCollision, const bool bMovementComp, const bool bMesh) const
{
	if (bCollision == true)
	{
		if (CollisionComponent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][ShowSkully] CollisionComponent = nullptr"));
		}
		else
		{
			CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
	}
	
	if (bMovementComp == true)
	{
		if (SkullyMovementComponent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][ShowSkully] SkullyMovementComponent = nullptr"));
		}
		else
		{
			SkullyMovementComponent->SetComponentTickEnabled(true);
		}
	}
	
	if (bMesh == true)
	{
		if (BoneMesh == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][ShowSkully] BoneMesh = nullptr"));
		}
		else
		{
			BoneMesh->SetVisibility(true);
		}
		if (ClayMesh == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Skully.cpp][ShowSkully] ClayMesh = nullptr"));
		}
		else
		{
			ClayMesh->SetVisibility(true);
		}
	}
}
