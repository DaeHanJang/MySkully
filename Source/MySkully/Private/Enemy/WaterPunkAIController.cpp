#include "Enemy/WaterPunkAIController.h"

void AWaterPunkAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (WaterPunkBehaviorTree == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][OnPossess] WaterPunkBehaviorTree = nullptr"))
		return;
	}
	
	RunBehaviorTree(WaterPunkBehaviorTree);
}
