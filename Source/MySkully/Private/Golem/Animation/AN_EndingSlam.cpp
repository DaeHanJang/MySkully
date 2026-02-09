#include "Golem/Animation/AN_EndingSlam.h"

#include "Golem/StrongGolem.h"

void UAN_EndingSlam::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_EndingSlam.cpp][Notify] Golem = nullptr"));
		return;
	}
	
	Golem->SetSlamEnding(true);
}
