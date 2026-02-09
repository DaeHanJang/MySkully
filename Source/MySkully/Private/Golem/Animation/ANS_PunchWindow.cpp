#include "Golem/Animation/ANS_PunchWindow.h"

#include "Golem/StrongGolem.h"

void UANS_PunchWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (MeshComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_PunchWindow.cpp][NotifyBegin] MeshComp = nullptr"));
		return;
	}
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_PunchWindow.cpp][NotifyBegin] StrongGolem = nullptr"));
		return;
	}
	
	Golem->BeginPunchWindow();
}

void UANS_PunchWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (MeshComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_PunchWindow.cpp][NotifyEnd] MeshComp = nullptr"));
		return;
	}
	
	AStrongGolem* Golem = Cast<AStrongGolem>(MeshComp->GetOwner());
	if (Golem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_PunchWindow.cpp][NotifyEnd] StrongGolem = nullptr"));
		return;
	}
	
	Golem->EndPunchWindow();
}
