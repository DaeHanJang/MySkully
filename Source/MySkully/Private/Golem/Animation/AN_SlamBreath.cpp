#include "Golem/Animation/AN_SlamBreath.h"

#include "Golem/StrongGolem.h"

void UAN_SlamBreath::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (MeshComp == nullptr)
	{
		return;
	}
	if (MeshComp->GetOwner() == nullptr)
	{
		return;
	}
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		return;
	}
	Golem->FireSlamBreath();
}
