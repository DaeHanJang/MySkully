#include "Golem/Animation/ANFinishSlam.h"

#include "Golem/StrongGolem.h"

void UANFinishSlam::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANFinishSlam.cpp][Notify] Golem = nullptr"));
		return;
	}
	
	Golem->SetSlam(false);
}
