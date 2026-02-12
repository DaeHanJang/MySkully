#include "Golem/Animation/ANS_ChargingSlam.h"

#include "Golem/StrongGolem.h"
void UANS_ChargingSlam::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_ChargingSlam.cpp][NotifyEnd] Golem = nullptr"));
		return;
	}
	Golem->SetSlamPower(Golem->GetSlamPower() + FrameDeltaTime);
}

void UANS_ChargingSlam::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_ChargingSlam.cpp][NotifyEnd] Golem = nullptr"));
		return;
	}
	if (Golem->GetSlamEnding() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_ChargingSlam.cpp][NotifyEnd] bSlamEnding = true"));
		return;
	}
	
	Golem->PlayAnimMontage(Golem->GetSlamEndMontage());
}
