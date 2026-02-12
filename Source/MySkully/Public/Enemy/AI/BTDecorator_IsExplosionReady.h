#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsExplosionReady.generated.h"

UCLASS()
class MYSKULLY_API UBTDecorator_IsExplosionReady : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_IsExplosionReady();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
};
