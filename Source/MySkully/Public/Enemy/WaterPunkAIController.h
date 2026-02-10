#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WaterPunkAIController.generated.h"

UCLASS()
class MYSKULLY_API AWaterPunkAIController : public AAIController
{
	GENERATED_BODY()
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = true))
	TObjectPtr<UBehaviorTree> WaterPunkBehaviorTree;
	
};
