#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYSKULLY_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHealthComponent();

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(const float Value) { Health = Value; }
	
	void LoseHealth(const float Amount);
	void GainHealth();
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess="true"))
	float Health = 100.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess="true"))
	float HealAmount = 1.0f;
	
};
