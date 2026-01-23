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

protected:
	virtual void BeginPlay() override;
	
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Value) { Health = Value; }

public:
	void LoseHealth(float Amount);
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Health")
	float Health = 100.0f;
};
