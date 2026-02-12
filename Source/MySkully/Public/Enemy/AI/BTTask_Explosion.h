#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Explosion.generated.h"

UCLASS()
class MYSKULLY_API UBTTask_Explosion : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_Explosion();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
