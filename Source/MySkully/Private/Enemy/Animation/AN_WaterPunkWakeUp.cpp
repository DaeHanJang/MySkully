#include "Enemy/Animation/AN_WaterPunkWakeUp.h"

#include "Enemy/WaterPunk.h"

void UAN_WaterPunkWakeUp::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	AWaterPunk* WaterPunk = Cast<AWaterPunk>(MeshComp->GetOwner());
	if (WaterPunk == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_WaterPunkWakeUp.cpp][Notify] WaterPunk = nullptr"));
		return;
	}
	
	WaterPunk->RequestWakeUp();
}
