#include "Enemy/WaterPunk.h"

#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Enemy/WaterPunkAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AWaterPunk::AWaterPunk()
{
	PrimaryActorTick.bCanEverTick = false;

	// 콜리전
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(160.0f, 160.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
	
	// 애로우 컴포넌트
	GetArrowComponent()->SetArrowLength(150.0f);
	GetArrowComponent()->PrimaryComponentTick.bCanEverTick = false;
	
	// 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Character/WaterPunk/WaterPunk.WaterPunk"));
	if (MeshAsset.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -140.0f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(TEXT("/Game/Character/WaterPunk/Animation/ABP_WaterPunk.ABP_WaterPunk_C"));
		if (AnimClass.Succeeded() == true)
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			GetMesh()->SetAnimInstanceClass(AnimClass.Class);
		}
	}
	
	// 무브먼트
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 1200.0f;
	
	// 감지 컴포넌트
	WaterPunkPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("WaterPunkPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1200.0f;
	SightConfig->LoseSightRadius = 1500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	WaterPunkPerception->ConfigureSense(*SightConfig);
	WaterPunkPerception->SetDominantSense(UAISense_Sight::StaticClass());
	WaterPunkPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AWaterPunk::OnTargetPerceptionUpdated);
	
	// 캐릭터 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AIControllerClass = AWaterPunkAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWaterPunk::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWaterPunk::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed() == true)
	{
		
	}
	else
	{
		
	}
}
