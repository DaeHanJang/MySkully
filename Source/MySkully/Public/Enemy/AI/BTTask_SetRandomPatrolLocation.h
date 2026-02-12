#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_SetRandomPatrolLocation.generated.h"

UCLASS()
class MYSKULLY_API UBTTask_SetRandomPatrolLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_SetRandomPatrolLocation();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float SearchRadius = 800.0f;
		
};
