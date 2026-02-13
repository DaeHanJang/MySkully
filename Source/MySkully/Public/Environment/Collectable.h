#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Collectable.generated.h"

class USphereComponent;

UCLASS()
class MYSKULLY_API ACollectable : public AActor
{
	GENERATED_BODY()
	
public:
	FORCEINLINE uint8 GetScore() const { return Score; }
	
	ACollectable();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnCollectableBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	void UpdateRotation();
	void RequestDestroy();
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CollectableMesh;
	
	// Collectable
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	uint8 Score;
	
	FTimerHandle RotatorTimerHandle;
	
};
