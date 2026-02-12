#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WaterPunkAIController.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;

UCLASS()
class MYSKULLY_API AWaterPunkAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AWaterPunkAIController();
	
	UFUNCTION(BlueprintCallable)
	void StartWaterPunkAI();
	UFUNCTION(BlueprintCallable)
	void StopWaterPunkAI();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
private:
	// AI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = true))
	TObjectPtr<UBehaviorTree> WaterPunkBehaviorTree;
	
	bool bAIStarted;
	
	// Perception
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> WaterPunkPerception;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
};
