#include "Golem/Animation/AN_FinishSlam.h"

#include "Golem/StrongGolem.h"

void UAN_FinishSlam::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_FinishSlam.cpp][Notify] Golem = nullptr"));
		return;
	}
	
	Golem->SetSlamPower(0.0f);
	Golem->SetSlamEnding(false);
	Golem->SetSlam(false);
}
