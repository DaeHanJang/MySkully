#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WaterPunk.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;
class UAIPerceptionComponent;
class UStateTreeComponent;

UCLASS()
class MYSKULLY_API AWaterPunk : public ACharacter
{
	GENERATED_BODY()

public:
	AWaterPunk();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> WaterPunkPerception;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
};
