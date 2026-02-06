#include "Golem/StrongGolem.h"

#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AStrongGolem::AStrongGolem()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 콜리전
	GetCapsuleComponent()->InitCapsuleSize(250.0f, 400.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
	
	// 애로우 컴포넌트
	GetArrowComponent()->SetArrowLength(150.0f);
	GetArrowComponent()->PrimaryComponentTick.bCanEverTick = false;
	
	// 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Character/Strong/Strong.Strong"));
	if (MeshAsset.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(-100.0f, 0.0f, -390.0f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->PrimaryComponentTick.bAllowTickBatching = false;
	}
	
	// 스프링 암
	CameraBoom->TargetArmLength = 2500.0f;
	
	// 상호작용 콜리전
	InteractionBox->InitBoxExtent(FVector(500.0f));
	
	// 무브먼트 컴포넌트
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 1400.0f;
	GetCharacterMovement()->JumpZVelocity = 1500.0f;
}

void AStrongGolem::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SetupPlayerInputComponent] EnhancedInputComponent = nullptr"));
		return;
	}
	
	// 특수 기능1(펀치) 맵핑
	if (PrimaryInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SetupPlayerInputComponent] PrimaryInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(PrimaryInputAction, ETriggerEvent::Triggered, this, &AGolemCharacter::Primary);
	}
	// 특수 기능2(슬램) 맵핑
	if (SecondaryInputAction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SetupPlayerInputComponent] SecondaryInputAction = nullptr"));
	}
	else
	{
		EIC->BindAction(SecondaryInputAction, ETriggerEvent::Started, this, &AGolemCharacter::Secondary);
		EIC->BindAction(SecondaryInputAction, ETriggerEvent::Completed, this, &AStrongGolem::StopSecondary);
	}
}

void AStrongGolem::DismountAction_Implementation()
{
	Super::DismountAction_Implementation();
}

void AStrongGolem::PrimaryAction_Implementation()
{
	Super::PrimaryAction_Implementation();
	
	if (bPunch == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][PrimaryAction_Implementation] bPunch = true"));
		return;
	}
	if (bSlam == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][PrimaryAction_Implementation] bSlam = true"));
		return;
	}
	
	bPunch = true;
	const uint8 Index = FMath::RandRange(0, 2);
	switch (Index)
	{
	case 0:
		PlayAnimMontage(PunchMontage, 1.0f, TEXT("Punch1"));
		break;
	case 1:
		PlayAnimMontage(PunchMontage, 1.0f, TEXT("Punch2"));
		break;
	case 2:
		PlayAnimMontage(PunchMontage, 1.0f, TEXT("Punch3"));
		break;
	default:
		break;
	}
}

void AStrongGolem::SecondaryAction_Implementation()
{
	Super::SecondaryAction_Implementation();
	
	if (bPunch == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SecondaryAction_Implementation] bPunch = true"));
		return;
	}
	if (bSlam == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SecondaryAction_Implementation] bSlam = true"));
		return;
	}
	
	bSlam = true;
	PlayAnimMontage(SlamStartMontage);
}

void AStrongGolem::StopSecondary(const FInputActionValue& Value)
{
	if (bPunch == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] bPunch = true"));
		return;
	}
	if (bSlam == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] bSlam = false"));
		return;
	}
	if (bSlamEnding == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] bSlamEnding = false"));
		return;
	}
	if (GetMesh()->GetAnimInstance()->Montage_GetCurrentSection(GetCurrentMontage()) == TEXT("End"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] CurrentMontage is Slam And Section Name is End"));
		return;
	}
	
	StopAnimMontage(GetCurrentMontage());
	PlayAnimMontage(SlamEndMontage, 1.0f, TEXT("End"));
}
