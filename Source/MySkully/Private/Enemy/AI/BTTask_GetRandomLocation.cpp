#include "Enemy/AI/BTTask_GetRandomLocation.h"

UBTTask_GetRandomLocation::UBTTask_GetRandomLocation()
{
	NodeName = TEXT("BTTask_GetRandomLocation");
}

EBTNodeResult::Type UBTTask_GetRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon)
	{
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::Succeeded;
}
