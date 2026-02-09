#include "Golem/StrongGolem.h"

#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Environment/DestructibleTile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SkullyPlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Golem/Animation/GolemAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

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
		// 애니메이션
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(TEXT("/Game/Character/Strong/Animation/APB_StrongGolem.APB_StrongGolem_C"));
		if (AnimClass.Succeeded() == true)
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			GetMesh()->SetAnimInstanceClass(AnimClass.Class);
		}
	}
	
	// 스프링 암
	CameraBoom->TargetArmLength = 2500.0f;
	
	// 상호작용 콜리전
	InteractionBox->InitBoxExtent(FVector(500.0f));
	
	// 무브먼트 컴포넌트
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 1400.0f;
	GetCharacterMovement()->JumpZVelocity = 1500.0f;
	
	// 펀치 콜리전
	PunchCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PunchCollision"));
	PunchCollision->SetupAttachment(GetMesh(), TEXT("R_DUMMY_JNTSocket"));
	PunchCollision->InitSphereRadius(50.0f);
	
	PunchCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PunchCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PunchCollision->SetGenerateOverlapEvents(true);
	PunchCollision->OnComponentBeginOverlap.AddDynamic(this, &AStrongGolem::OnFistOverlap);
	PunchCollision->PrimaryComponentTick.bCanEverTick = false;
}

void AStrongGolem::PossessedBy(AController* NewController)
{
	UE_LOG(LogTemp, Warning, TEXT("Golem PossessedBy: New=%s"), *GetNameSafe(NewController));
	
	Super::PossessedBy(NewController);
	
	UE_LOG(LogTemp, Warning, TEXT("Golem PossessedBy after Super. bExist=%d"), bExist);
	
	if (bExist == false)
	{
		bExist = true;
	}
	else
	{
		Eat();
	}
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
	
	UGolemAnimInstance* AnimInst =  Cast<UGolemAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][DismountAction_Implementation] GolemAnimInstance = nullptr"));
	}
	else
	{
		AnimInst->SetDismount(true);
	}
}

void AStrongGolem::DespawnAction_Implementation()
{
	Super::DespawnAction_Implementation();
	
	UGolemAnimInstance* AnimInst =  Cast<UGolemAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][DespawnAction_Implementation] GolemAnimInstance = nullptr"));
	}
	else
	{
		AnimInst->SetDespawn(true);
	}
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
	if (PunchMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][PrimaryAction_Implementation] PunchMontage = nullptr"));
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
	if (SlamStartMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][SecondaryAction_Implementation] SlamStartMontage = nullptr"));
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
	if (GetMesh()->GetAnimInstance()->Montage_GetCurrentSection(GetCurrentMontage()) == TEXT("End"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] Already CurrentMontage is EndSlam And Section Name is End"));
		return;
	}
	if (bSlamEnding == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][StopSecondary] bSlamEnding = true"));
		return;
	}
	
	StopAnimMontage(GetCurrentMontage());
	PlayAnimMontage(SlamEndMontage, 1.0f, TEXT("End"));
}

void AStrongGolem::Eat()
{
	UE_LOG(LogTemp, Warning, TEXT("Golem Eat called"));
	Super::Eat();
	
	if (EatMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][Eat] EatMontage = nullptr"));
		return;
	}
	
	ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][Eat] SkullyPlayerController = nullptr"));
		return;
	}
	
	const FVector DirToSkully = (PC->GetSkully()->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const float Dot = FVector::DotProduct(GetActorForwardVector(), DirToSkully);
	
	PC->GetSkully()->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("L_DUMMY_JNTSocket"));
	
	if (Dot >= 0.0f)
	{
		PlayAnimMontage(EatMontage, 1.0f, TEXT("Front"));
	}
	else
	{
		PlayAnimMontage(EatMontage, 1.0f, TEXT("Back"));
	}
}

void AStrongGolem::BeginPunchWindow()
{
	UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][BeginPunchWindow]"));
	HitActorsThisPunch.Reset();
	PunchCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
void AStrongGolem::EndPunchWindow()
{
	UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][EndPunchWindow]"));
	PunchCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AStrongGolem::OnFistOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][OnFistOverlap]"));
	if (OtherActor == nullptr || OtherActor == this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][OnFistOverlap] OtherActor = nullptr"));
		return;
	}
	if (HitActorsThisPunch.Contains(OtherActor) == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][OnFistOverlap] OtherActor in HitActorsThisPunch"));
		return;
	}
	HitActorsThisPunch.Add(OtherActor);
	ADestructibleTile* Rock = Cast<ADestructibleTile>(OtherActor);
	if (Rock == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StrongGolem.cpp][OnFistOverlap] DestructibleTile = nullptr"));
		return;
	}
	
	const FVector HitPos = SweepResult.ImpactPoint.IsNearlyZero() == true ? OtherActor->GetActorLocation() : FVector(SweepResult.ImpactPoint);
	Rock->ApplyPunchAt(HitPos, 1e10f, 60.0f);
}