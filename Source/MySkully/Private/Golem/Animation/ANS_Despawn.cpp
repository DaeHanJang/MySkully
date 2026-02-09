#include "Golem/Animation/ANS_Despawn.h"

#include "GameFramework/SkullyPlayerController.h"
#include "Golem/GolemCharacter.h"
#include "Skully/Skully.h"

void UANS_Despawn::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	const APawn* Owner = Cast<APawn>(MeshComp->GetOwner());
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_Despawn.cpp][NotifyBegin] MeshComp->GetOwner = nullptr"));
		return;
	}
	
	const AGolemCharacter* GolemCharacter = Cast<AGolemCharacter>(Owner);
	if (GolemCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_Despawn.cpp][NotifyBegin] Owner = nullptr"));
		return;
	}
	GolemCharacter->Despawn();
	
	const ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(Owner->GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_Despawn.cpp][NotifyBegin] SkullyPlayerController = nullptr"));
	}
	else
	{
		PC->GetSkully()->DespawnGolem();
	}
}

void UANS_Despawn::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	AActor* Owner = MeshComp->GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_Despawn.cpp][NotifyTick] MeshComp->GetOwner() = nullptr"));
		return;
	}
	Owner->SetActorLocation(Owner->GetActorLocation() + FVector::DownVector * DownSpeed);
}

void UANS_Despawn::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	AGolemCharacter* GolemCharacter = Cast<AGolemCharacter>(MeshComp->GetOwner());
	if (GolemCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_Despawn.cpp][NotifyBegin] Owner = nullptr"));
		return;
	}
	GolemCharacter->DelayDestroy();
}
