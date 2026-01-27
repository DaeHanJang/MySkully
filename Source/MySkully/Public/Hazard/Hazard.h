#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Hazard.generated.h"

class ASkully;
class UBoxComponent;

UCLASS()
class MYSKULLY_API AHazard : public AActor
{
	GENERATED_BODY()
	
public:	
	AHazard();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
								   int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	UBoxComponent* BoxComponent;

	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	UStaticMeshComponent* PlaneMesh;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (AllowPrivateAccess="true"))
	float DamagePerTick = 25.0f;
	// 데미지 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (AllowPrivateAccess="true"))
	float DamageTickInterval = 1.0f;
	
private:
	void DealDamageTick() const;
	
	TWeakObjectPtr<ASkully> OverlappingSkully;
	FTimerHandle DamageTimerHandle;
};
