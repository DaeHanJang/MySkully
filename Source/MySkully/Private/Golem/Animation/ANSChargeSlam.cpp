#include "Golem/Animation/ANSChargeSlam.h"

#include "Golem/StrongGolem.h"

void UANSChargeSlam::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	ChargeTime += FrameDeltaTime;
}

void UANSChargeSlam::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANSChargeSlam.cpp][NotifyEnd] Golem = nullptr"));
		return;
	}
	
	Golem->PlayAnimMontage(Golem->GetSlamEndMontage());
	Golem->SetSlamPower(ChargeTime);
}
