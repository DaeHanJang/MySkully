#include "Enemy/Animation/AN_WaterPunkDie.h"

#include "Enemy/WaterPunk.h"

void UAN_WaterPunkDie::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (MeshComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_WaterPunkDie][Notify] MeshComp = nullptr"));
		return;
	}
	if (MeshComp->GetOwner() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_WaterPunkDie][Notify] Owner = nullptr"));
		return;
	}
	AWaterPunk* WaterPunk = Cast<AWaterPunk>(MeshComp->GetOwner());
	if (WaterPunk == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_WaterPunkDie][Notify] WaterPunk = nullptr"));
		return;
	}
	
	WaterPunk->StartDeathSink();
}
