#include "Enemy/WaterPunk.h"

#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/HealthComponent/HealthComponent.h"
#include "Enemy/WaterPunkAIController.h"
#include "Enemy/Animation/WaterPunkAnimInstance.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Golem/GolemCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skully/Skully.h"

AWaterPunk::AWaterPunk()
{
	PrimaryActorTick.bCanEverTick = false;

	// 콜리전
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(160.0f, 160.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AWaterPunk::OnHit);
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
		
	// 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Character/WaterPunk/WaterPunk.WaterPunk"));
	if (MeshAsset.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -150.0f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(TEXT("/Game/Character/WaterPunk/Animation/ABP_WaterPunk.ABP_WaterPunk_C"));
		if (AnimClass.Succeeded() == true)
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			GetMesh()->SetAnimInstanceClass(AnimClass.Class);
		}
	}
	
	// 무브먼트
	GetCharacterMovement()->SetComponentTickEnabled(false);
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 340.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	
	// 캐릭터 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AIControllerClass = AWaterPunkAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWaterPunk::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return;
	}
	
	const ASkully* Skully = Cast<ASkully>(OtherActor);
	if (Skully != nullptr && OtherComp == Skully->GetRootComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][OnHit] On Hit Skully"));
		
		UHealthComponent* HealthComponent = Skully->FindComponentByClass<UHealthComponent>();
		if (HealthComponent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][OnHit] HealthComponent = nullptr"));
			return;
		}
		
		HealthComponent->LoseHealth(100.0f);
	}
}

void AWaterPunk::RequestStartAI()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] AIController = nullptr"));
		return;
	}
	AWaterPunkAIController* WaterPunkAIController = Cast<AWaterPunkAIController>(AIController);
	if (WaterPunkAIController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] WaterPunkAIController = nullptr"));
		return;
	}
	
	WaterPunkAIController->StartWaterPunkAI();
}

void AWaterPunk::RequestStopAI()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] AIController = nullptr"));
		return;
	}
	AWaterPunkAIController* WaterPunkAIController = Cast<AWaterPunkAIController>(AIController);
	if (WaterPunkAIController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] WaterPunkAIController = nullptr"));
		return;
	}
	
	WaterPunkAIController->StopWaterPunkAI();
}

void AWaterPunk::PlayWakeUp()
{
	UWaterPunkAnimInstance* WaterPunkAnimInstance = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (WaterPunkAnimInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][WakeUp] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	WaterPunkAnimInstance->SetWakeUp(true);
}

void AWaterPunk::RequestWakeUp()
{
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GetCharacterMovement()->SetComponentTickEnabled(true);
	GetCharacterMovement()->GravityScale = 1.0f;
	RequestStartAI();
}

void AWaterPunk::SeePlayer()
{
	UWaterPunkAnimInstance* AnimInst = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][SeePlayer] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	AnimInst->SetSee(true);
}

void AWaterPunk::LostPlayer()
{
	UWaterPunkAnimInstance* AnimInst = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][SeePlayer] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	AnimInst->SetSee(false);
}

void AWaterPunk::RequestStartChasing()
{
	UWaterPunkAnimInstance* AnimInst = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	AnimInst->SetChase(true);
}

void AWaterPunk::RequestStopChasing()
{
	UWaterPunkAnimInstance* AnimInst = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (AnimInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][RequestStartAI] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	AnimInst->SetChase(false);
}

void AWaterPunk::Hit()
{
	UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][Die]"));
	
	if (GetMesh() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][Die] GetMesh = nullptr"));
		return;
	}
	if (GetMesh()->GetAnimInstance() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][Die] GetAnimInstance = nullptr"));
		return;
	}
	UWaterPunkAnimInstance* WaterPunkAnimInstance = Cast<UWaterPunkAnimInstance>(GetMesh()->GetAnimInstance());
	if (WaterPunkAnimInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][Die] WaterPunkAnimInstance = nullptr"));
		return;
	}
	
	GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
	SetActorScale3D(FVector(1.0f));
	RequestStopAI();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetComponentTickEnabled(false);
	WaterPunkAnimInstance->SetHit(true);
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
}

void AWaterPunk::StartDeathSink()
{
	if (GetWorldTimerManager().IsTimerActive(DestroyTimerHandle) == false)
	{
		DeathSinkElapsedTime = 0.0f;
		GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AWaterPunk::UpdateDeathSink, 0.01f, true, 0.0f);
	}
}

void AWaterPunk::UpdateDeathSink()
{
	DeathSinkElapsedTime += 0.01f;
	
	SetActorLocation(GetActorLocation() + GetActorUpVector() * -1.0f);
	
	if (DeathSinkElapsedTime >= 2.0f)
	{
		GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
		Destroy();
	}
}
	
void AWaterPunk::Explosion()
{
	if (GetWorldTimerManager().IsTimerActive(ExplosionTimerHandle) == false)
	{
		GetWorldTimerManager().SetTimer(ExplosionTimerHandle, this, &AWaterPunk::UpdateExplosionScale, 0.01f, true, 0.0f);
	}
}

void AWaterPunk::UpdateExplosionScale()
{
	SetActorScale3D(GetActorScale3D() + FVector(0.001f));
	
	if (GetActorScale3D().X >= 2.0f)
	{
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][UpdateExplosionScale] AIController = nullptr"));
			return;
		}
		if (GetWorld() == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WaterPunk][UpdateExplosionScale] World = nullptr"));
			return;
		}
		
		TArray<FOverlapResult> Overlaps;
		CollectHitActorsWithOcclusionFilter(GetActorLocation(), GetCapsuleComponent()->GetScaledCapsuleRadius(), Overlaps);
		
		SetActorScale3D(FVector(1.0f));
		GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
		UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
	}
}

void AWaterPunk::CollectHitActorsWithOcclusionFilter(const FVector& CenterPos, float SphereRadius, TArray<FOverlapResult>& Overlaps)
{
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BreathOverlap), false);
	QueryParams.AddIgnoredActor(this);
		
	const bool bHit = GetWorld()->OverlapMultiByObjectType(Overlaps, CenterPos, FQuat::Identity, ObjQuery, FCollisionShape::MakeSphere(SphereRadius), QueryParams);
	
	//DrawDebugSphere(GetWorld(), CenterPos, SphereRadius, 12, bHit == true ? FColor::Red : FColor::Green, false, 0.5f);
	
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Other = R.GetActor();
		if (Other == nullptr || Other == this)
		{
			continue;
		}
		if (R.GetComponent() != Other->GetRootComponent())
		{
			continue;
		}
		
		if (HasLineOfSlamBreathToActor(GetActorLocation(), Other) == false)
		{
			continue;
		}
		
		ASkully* Skully = Cast<ASkully>(Other);
		if (Skully != nullptr)
		{
			UHealthComponent* HealthComponent = Skully->FindComponentByClass<UHealthComponent>();
			if (HealthComponent == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[WaterPunk.cpp][CollectHitActorsWithOcclusionFilter] HealthComponent = nullptr"));
				return;
			}
			
			HealthComponent->LoseHealth(100.0f);
		}
		AGolemCharacter* Golem = Cast<AGolemCharacter>(Other);
		if (Golem != nullptr)
		{
			UHealthComponent* HealthComponent = Golem->FindComponentByClass<UHealthComponent>();
			if (HealthComponent == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("[WaterPunk.cpp][CollectHitActorsWithOcclusionFilter] HealthComponent = nullptr"));
				return;
			}
			
			HealthComponent->LoseHealth(25.0f);
		}
	}
}

bool AWaterPunk::HasLineOfSlamBreathToActor(const FVector& From, AActor* Target) const
{
	const FVector To = Target->GetActorLocation();
	
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SlamBreathLOS), false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Target);
	
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
	
	if (bBlocked == false)
	{
		return true;
	}
	
	return false;
}
