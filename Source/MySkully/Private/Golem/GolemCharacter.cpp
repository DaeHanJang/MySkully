#include "Golem/GolemCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/BoxComponent.h"
#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gollem/GollemCharacter.h"
#include "Skully/SkullyCameraComponent.h"

AGolemCharacter::AGolemCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 스프링 암
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));;
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;
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
	
	// 상호작용 콜리전
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(GetRootComponent());
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetGenerateOverlapEvents(true);
	InteractionBox->PrimaryComponentTick.bCanEverTick = false;
	
	// 헬스 컴포넌트
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 웅덩이 상호작용 컴포넌트
	ClayMoundReactiveComponent = CreateDefaultSubobject<UClayMoundReactiveComponent>(TEXT("ClayMoundReactiveComponent"));
	ClayMoundReactiveComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 무브먼트 컴포넌트
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// 캐릭터 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AGolemCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	PlaySpawnCameraSequence();
}
void AGolemCharacter::PlaySpawnCameraSequence()
{
	if (CameraBoom == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][PlaySpawnCameraSequence] CameraBoom = nullptr"));
		return;
	}
	
	bIsPlayingCameraSequence = true;
	CachedCameraBoomRotation = CameraBoom->GetRelativeRotation();
	SpawnCameraBoomRotation = CameraBoom->GetRelativeRotation() + FRotator(0.0f, 180.0f, 0.0f);
	SpawnCameraBoomRotationSpeed = 0.0f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetRelativeRotation(SpawnCameraBoomRotation);
	
	GetWorldTimerManager().SetTimer(SpawnCameraSequenceTimerHandle, this, &AGolemCharacter::UpdateSpawnCameraSequence, 0.01f, true, 2.0f);
}
void AGolemCharacter::UpdateSpawnCameraSequence()
{
	if (CameraBoom == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][UpdateSpawnCameraSequence] CameraBoom = nullptr"));
		bIsPlayingCameraSequence = false;
		CachedCameraBoomRotation = FRotator::ZeroRotator;
		GetWorldTimerManager().ClearTimer(SpawnCameraSequenceTimerHandle);
		return;
	}
	
	SpawnCameraBoomRotationSpeed += 0.01f;
	const float Alpha = FMath::Clamp(SpawnCameraBoomRotationSpeed / 0.5f, 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	const FRotator NewRot = FMath::Lerp(SpawnCameraBoomRotation, CachedCameraBoomRotation, SmoothAlpha);
	CameraBoom->SetRelativeRotation(NewRot);
	
	if (Alpha >= 1.0f)
	{
		Controller->SetControlRotation(CameraBoom->GetComponentRotation());
		
		bIsPlayingCameraSequence = false;
		CachedCameraBoomRotation = FRotator::ZeroRotator;
		CameraBoom->bUsePawnControlRotation = true;
		GetWorldTimerManager().ClearTimer(SpawnCameraSequenceTimerHandle);
	}
}

void AGolemCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][PossessedBy] PlayerController = nullptr"));
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][PossessedBy] LocalPlayer = nullptr"));
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][PossessedBy] EnhancedInputLocalPlayerSubsystem = nullptr"));
		return;
	}
	
	// IMC 연결
	EILPS->AddMappingContext(InputMappingContext, 0);
}

void AGolemCharacter::UnPossessed()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][UnPossessed] PlayerController = nullptr"));
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][UnPossessed] LocalPlayer = nullptr"));
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][UnPossessed] EnhancedInputLocalPlayerSubsystem = nullptr"));
		return;
	}
	
	// IMC 해제
	EILPS->RemoveMappingContext(InputMappingContext);
	
	Super::UnPossessed();
}

void AGolemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][SetupPlayerInputComponent] EnhancedInputComponent = nullptr"));
		return;
	}
	
	// 이동 맵핑
	if (MoveInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][SetupPlayerInputComponent] MoveInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AGolemCharacter::Move);
	}
	// 시점 맵핑
	if (LookInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][SetupPlayerInputComponent] LookInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AGolemCharacter::Look);
	}
	// 점프 맵핑
	if (JumpInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][SetupPlayerInputComponent] JumpInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AGolemCharacter::StartJump);
		EIC->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &AGolemCharacter::StopJump);
	}
	// 하차 맵핑
	if (DismountInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][SetupPlayerInputComponent] DismountInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(DismountInputAction, ETriggerEvent::Triggered, this, &AGolemCharacter::Dismount);
	}
}

void AGolemCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	auto* MoveComp = GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][OnMovementModeChanged] CharacterMovementComponent = nullptr"));
		return;
	}
	
	// 현재 공중인 경우 중력 스케일 적용
	if (MoveComp->IsFalling() == true)
	{
		bDescending = false;
		MoveComp->GravityScale = GravityScaleAscending;
		if (GetWorldTimerManager().IsTimerActive(FallingTimerHandle) == false)
		{
			GetWorldTimerManager().SetTimer(FallingTimerHandle, this, &AGolemCharacter::CheckFallingApex, 0.03f, true);
		}
	}
	// 이전 프레임에 공중이었던 경우 중력 스케일 복원
	else if (PrevMovementMode == MOVE_Falling)
	{
		MoveComp->GravityScale = 1.0f;
		if (GetWorldTimerManager().IsTimerActive(FallingTimerHandle) == true)
		{
			GetWorldTimerManager().ClearTimer(FallingTimerHandle);
		}
	}
}

void AGolemCharacter::OnTakeDamage_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][OnTakeDamage_Implementation] HP: %f"), HealthComponent->GetHealth());
}
void AGolemCharacter::OnDeath_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][OnDeath_Implementation] HP: %f"), HealthComponent->GetHealth());
}
void AGolemCharacter::OnTakeHealth_Implementation()
{
}

void AGolemCharacter::OnEnterClayMound_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][OnEnterClayMound_Implementation] EnterClayMound"));
}
void AGolemCharacter::OnExitClayMound_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][OnExitClayMound_Implementation] ExitClayMound"));
}

void AGolemCharacter::CheckFallingApex()
{
	if (bDescending == true)
	{
		return;
	}
	
	auto* MoveComp = GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][CheckFallingApex] CharacterMovementComponent = nullptr"));
		return;
	}
	
	// 낙하
	if (GetVelocity().Z < 0.0f)
	{
		bDescending = true;
		MoveComp->GravityScale = GravityScaleDescending;
	}
}

void AGolemCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Move] Controller = nullptr"));
		return;
	}
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Move] bIsPlayingCameraSequence = true"));
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

void AGolemCharacter::Look(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Look] Controller = nullptr"));
		return;
	}
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Look] bIsPlayingCameraSequence = true"));
		return;
	}
	
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
	
	if (FollowCameraCollision == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Look] FollowCameraCollision = nullptr"));
	}
	else
	{
		FollowCameraCollision->UpdateOverlaps();
	}
}

void AGolemCharacter::StartJump(const FInputActionValue& Value)
{
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][StartJump] bIsPlayingCameraSequence = true"));
		return;
	}
	
	Jump();
}
void AGolemCharacter::StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void AGolemCharacter::Dismount(const FInputActionValue& Value)
{
	if (ClayMoundReactiveComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Golem.cpp][Interact] ClayMoundReactiveComponent = nullptr"));
		return;
	}
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Dismount] bIsPlayingCameraSequence = true"));
		return;
	}
	
	DismountAction();
}
void AGolemCharacter::DismountAction_Implementation()
{
}

void AGolemCharacter::Primary(const FInputActionValue& Value)
{
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Primary] bIsPlayingCameraSequence = true"));
		return;
	}
	
	PrimaryAction();
}
void AGolemCharacter::PrimaryAction_Implementation()
{
}

void AGolemCharacter::Secondary(const FInputActionValue& Value)
{
	if (bIsPlayingCameraSequence == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GolemCharacter.cpp][Secondary] bIsPlayingCameraSequence = true"));
		return;
	}
	
	SecondaryAction();
}
void AGolemCharacter::SecondaryAction_Implementation()
{
}
