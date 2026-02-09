#include "Golem/Animation/AN_FinishPunch.h"

#include "Golem/StrongGolem.h"

void UAN_FinishPunch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                             const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_FinishPunch.cpp][Notify] Golem = nullptr"));
		return;
	}
	
	Golem->SetPunch(false);
}
