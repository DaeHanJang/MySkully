#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_GetRandomLocation.generated.h"

UCLASS()
class MYSKULLY_API UBTTask_GetRandomLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_GetRandomLocation();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
