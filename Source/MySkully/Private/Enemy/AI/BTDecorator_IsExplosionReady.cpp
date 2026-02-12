#include "Enemy/AI/BTDecorator_IsExplosionReady.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsExplosionReady::UBTDecorator_IsExplosionReady()
{
	NodeName = TEXT("BTDecorator_IsExplosionReady");
}

bool UBTDecorator_IsExplosionReady::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTDecorator_IsExplosionReady][CalculateRawConditionValue] BlackboardComponent = nullptr"));
		return false;
	}
	
	const float NextTime = BB->GetValueAsFloat("NextExplosionTime");
	const float Now = OwnerComp.GetWorld()->GetTimeSeconds();
	
	return Now >= NextTime;
}
