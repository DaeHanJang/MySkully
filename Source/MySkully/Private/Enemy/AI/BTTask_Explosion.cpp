#include "Enemy/AI/BTTask_Explosion.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/WaterPunk.h"

UBTTask_Explosion::UBTTask_Explosion()
{
	NodeName = TEXT("BTTask_Explosion");
}

EBTNodeResult::Type UBTTask_Explosion::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (AICon == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_Explosion][ExecuteTask] AIController = nullptr"));
		return EBTNodeResult::Failed;
	}
	APawn* Pawn = AICon->GetPawn();
	if (Pawn == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_Explosion][ExecuteTask] Pawn = nullptr"));
		return EBTNodeResult::Failed;
	}
	AWaterPunk* WaterPunk = Cast<AWaterPunk>(Pawn);
	if (WaterPunk == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_Explosion][ExecuteTask] WaterPunk = nullptr"));
		return EBTNodeResult::Failed;
	}
	
	WaterPunk->Explosion();
	return EBTNodeResult::Succeeded;
}
