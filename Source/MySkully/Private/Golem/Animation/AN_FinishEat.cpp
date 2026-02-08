#include "Golem/Animation/AN_FinishEat.h"

#include "GameFramework/SkullyPlayerController.h"
#include "Skully/Skully.h"

void UAN_FinishEat::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	const APawn* Owner = Cast<APawn>(MeshComp->GetOwner());
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_FinishEat.cpp][Notify] MeshComp->GetOwner = nullptr"));
		return;
	}
	
	const ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(Owner->GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_FinishEat.cpp][Notify] SkullyPlayerController = nullptr"));
	}
	else
	{
		PC->GetSkully()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		PC->GetSkully()->HideSkully(true, false);
	}
}
