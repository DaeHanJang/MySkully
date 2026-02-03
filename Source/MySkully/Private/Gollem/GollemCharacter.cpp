#include "Gollem/GollemCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Skully/SkullyCameraComponent.h"

class UEnhancedInputLocalPlayerSubsystem;

AGollemCharacter::AGollemCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
}

void AGollemCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	ResolveOptionalComponents();
}

void AGollemCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	ApplyInputMappingContext();
}

void AGollemCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	
	// 클라에서 Possess/Restart 타이밍에 IMC 적용이 누락되는 경우 보정
	ApplyInputMappingContext();
}

void AGollemCharacter::UnPossessed()
{
	RemoveInputMappingContext();
	
	Super::UnPossessed();
}

void AGollemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GollemCharacter] EnhancedInputComponent not fount %s"), *GetName());
		return;
	}
	
	if (IA_Move != nullptr)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AGollemCharacter::Move);
	}
	if (IA_Look != nullptr)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AGollemCharacter::Look);
	}
	if (IA_Jump != nullptr)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AGollemCharacter::StartJump);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AGollemCharacter::EndJump);
	}
	if (IA_Interact != nullptr)
	{
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AGollemCharacter::Interact);
	}
	if (IA_Primary != nullptr)
	{
		EIC->BindAction(IA_Primary, ETriggerEvent::Started, this, &AGollemCharacter::Primary);
	}
	if (IA_Secondary != nullptr)
	{
		EIC->BindAction(IA_Secondary, ETriggerEvent::Started, this, &AGollemCharacter::Secondary);
	}
}

void AGollemCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	auto* MoveComp = GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		return;
	}
	
	const EMovementMode NewMode = MoveComp->MovementMode;
	
	if (NewMode == MOVE_Falling)
	{
		bDescendingGravityApplied = false;
		MoveComp->GravityScale = GravityScaleAscending;
		StartFallingMonitor();
	}
	else if (PrevMovementMode == MOVE_Falling)
	{
		StopFallingMonitor();
		MoveComp->GravityScale = GravityScaleGrounded;
	}
}

void AGollemCharacter::ResolveOptionalComponents()
{
	if (InteractionBox == nullptr)
	{
		InteractionBox = FindComponentByClass<UBoxComponent>();
	}
	if (CameraBoom == nullptr)
	{
		CameraBoom = FindComponentByClass<USpringArmComponent>();
	}
	if (FollowCamera == nullptr)
	{
		FollowCamera = FindComponentByClass<USkullyCameraComponent>();
	}
}

void AGollemCharacter::ApplyInputMappingContext()
{
	if (bInputMappingContextApplied == true)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		return;
	}
	
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP == nullptr)
	{
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS == nullptr)
	{
		return;
	}
	
	if (IMC_Common != nullptr)
	{
		EILPS->AddMappingContext(IMC_Common, IMC_CommonPriority);
	}
	if (IMC_FormSpecific != nullptr)
	{
		EILPS->AddMappingContext(IMC_FormSpecific, IMC_FormPriority);
	}
	
	bInputMappingContextApplied = true;
}

void AGollemCharacter::RemoveInputMappingContext()
{
	if (bInputMappingContextApplied == false)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr)
	{
		bInputMappingContextApplied = false;
		return;
	}
	
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP != nullptr)
	{
		bInputMappingContextApplied = false;
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* EILPS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (EILPS != nullptr)
	{
		bInputMappingContextApplied = false;
		return;
	}
	
	if (IMC_Common != nullptr)
	{
		EILPS->RemoveMappingContext(IMC_Common);
	}
	if (IMC_FormSpecific != nullptr)
	{
		EILPS->RemoveMappingContext(IMC_FormSpecific);
	}
	
	bInputMappingContextApplied = false;
}

void AGollemCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller == nullptr)
	{
		return;
	}
	
	const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AGollemCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
	
	if (CameraBoxComponent != nullptr)
	{
		CameraBoxComponent->UpdateOverlaps();
	}
}

void AGollemCharacter::StartJump(const FInputActionValue& Value)
{
	Jump();
}

void AGollemCharacter::EndJump(const FInputActionValue& Value)
{
	StopJumping();
}

void AGollemCharacter::Interact(const FInputActionValue& Value)
{
	InteractAction();
}

void AGollemCharacter::Primary(const FInputActionValue& Value)
{
	PrimaryAction();
}

void AGollemCharacter::Secondary(const FInputActionValue& Value)
{
	SecondaryAction();
}

void AGollemCharacter::InteractAction_Implementation()
{
}

void AGollemCharacter::PrimaryAction_Implementation()
{
}

void AGollemCharacter::SecondaryAction_Implementation()
{
}

void AGollemCharacter::StartFallingMonitor()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FallingCheckTimer, this, &AGollemCharacter::CheckFallingApex, 0.03f, true);
	}
}

void AGollemCharacter::StopFallingMonitor()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallingCheckTimer);
	}
}

void AGollemCharacter::CheckFallingApex()
{
	if (bDescendingGravityApplied == true)
	{
		return;
	}
	
	auto* MoveComp = GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		return;
	}
	
	if (MoveComp->IsFalling() == false)
	{
		StopFallingMonitor();
		return;
	}
	
	const float ZVel = GetVelocity().Z;
	if (ZVel < 0.0f)
	{
		MoveComp->GravityScale = GravityScaleDescending;
		bDescendingGravityApplied = true;
	}
}

void AGollemCharacter::StartSpawnFrontCamera()
{
}

void AGollemCharacter::EndSpawnFrontCamera()
{
}

void AGollemCharacter::BeginSpawnFrontCameraReturn()
{
}

void AGollemCharacter::TickSpawnFrontCameraReturn()
{
}
