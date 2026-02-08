#include "Golem/Animation/AN_Dismount.h"

#include "GameFramework/SkullyPlayerController.h"
#include "Golem/Animation/GolemAnimInstance.h"
#include "Skully/Skully.h"

void UAN_Dismount::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	UGolemAnimInstance* GolemAnimInstance = Cast<UGolemAnimInstance>(MeshComp->GetAnimInstance());
	if (GolemAnimInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_Dismount.cpp][Notify] GolemAnimInstance = nullptr"));
	}
	else
	{
		GolemAnimInstance->SetDismount(false);
	}
	
	const APawn* Owner = Cast<APawn>(MeshComp->GetOwner());
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_Dismount.cpp][Notify] MeshComp->GetOwner = nullptr"));
		return;
	}
	const ASkullyPlayerController* PC = Cast<ASkullyPlayerController>(Owner->GetController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AN_Dismount.cpp][Notify] SkullyPlayerController = nullptr"));
	}
	else
	{
		PC->GetSkully()->DismountGolem(ZOffset);
	}
}
