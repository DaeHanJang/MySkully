#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WaterPunk.generated.h"

class USphereComponent;
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
	
	// AI
	void RequestStartAI();
	void RequestStopAI();
	void PlayWakeUp();
	void RequestWakeUp();
	void SeePlayer();
	void LostPlayer();
	void RequestStartChasing();
	void RequestStopChasing();
	void Hit();
	void StartDeathSink();
	void UpdateDeathSink();
	void Explosion();
	void UpdateExplosionScale();
	
	FTimerHandle DestroyTimerHandle;
	float DeathSinkElapsedTime;

	FTimerHandle ExplosionTimerHandle;
	
protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
		
private:
	void CollectHitActorsWithOcclusionFilter(const FVector& CenterPos, float SphereRadius, TArray<FOverlapResult>& Overlaps);
	bool HasLineOfSlamBreathToActor(const FVector& From, AActor* Target) const;

private:
	// Sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> ExplosionSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> DeathSound;
};
