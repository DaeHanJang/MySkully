#include "Golem/Animation/ANFinishPunch.h"

#include "Golem/StrongGolem.h"

void UANFinishPunch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANFinishPunch.cpp][Notify] Golem = nullptr"));
		return;
	}
	
	Golem->SetPunch(false);
}
