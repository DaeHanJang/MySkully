#include "Enemy/AI/BTTask_SetRandomPatrolLocation.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetRandomPatrolLocation::UBTTask_SetRandomPatrolLocation()
{
	NodeName = TEXT("BTTask_SetRandomPatrolLocation");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_SetRandomPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AICon = OwnerComp.GetAIOwner();
	if (AICon == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] AIController = nullptr"));
		return EBTNodeResult::Failed;
	}
	const APawn* Pawn = AICon->GetPawn();
	if (Pawn == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] Pawn = nullptr"));
		return EBTNodeResult::Failed;
	}
	UWorld* World = Pawn->GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] World = nullptr"));
		return EBTNodeResult::Failed;
	}
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] NavigationSystemV1 = nullptr"));
		return EBTNodeResult::Failed;
	}

	FNavLocation OutLocation;
	const bool bFound = NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), SearchRadius, OutLocation);
	if (bFound == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] GetRandomReachablePointInRadius = false"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_SetRandomPatrolLocation][ExecuteTask] BlackboardComponent = nullptr"));
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(BlackboardKey.SelectedKeyName, OutLocation.Location);
	return EBTNodeResult::Succeeded;
}
