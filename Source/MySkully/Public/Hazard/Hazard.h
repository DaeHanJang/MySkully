#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Hazard.generated.h"

class UPostProcessComponent;
class AGollemCharacter;
class ASkully;
class UBoxComponent;

UCLASS()
class MYSKULLY_API AHazard : public AActor
{
	GENERATED_BODY()
	
public:	
	AHazard();

protected:	
	UFUNCTION()
	void OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// Health
	void DealDamage() const;
	
private:
	// Collision(Root)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> CollisionComponent;

	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> SurfaceMesh;

	// Post Process
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PostProcess", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UPostProcessComponent> OverlapPostProcess;
	
	// Health
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta = (AllowPrivateAccess="true"))
	float Damage = 25.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta = (AllowPrivateAccess="true"))
	float DamageInterval = 1.0f;
	FTimerHandle DamageTimerHandle;
};
