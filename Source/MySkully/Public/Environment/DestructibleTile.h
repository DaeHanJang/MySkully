#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DestructibleTile.generated.h"

class UBoxComponent;
struct FChaosBreakEvent;
class UFieldSystemMetaDataFilter;

UCLASS()
class MYSKULLY_API ADestructibleTile : public AActor
{
	GENERATED_BODY()
	
public:
	ADestructibleTile();
	
	UFUNCTION(BlueprintCallable)
	void ApplyPunchAt(const FVector& PunchDir = FVector::ZeroVector, const FVector& WorldPos = FVector::ZeroVector, const float Strain = 1e10f, const float VelocityMag = 2500.0f);

private:
	UFUNCTION()
	void OnBreak(const FChaosBreakEvent& BreakingData);
	
	UFUNCTION()
	void UpdateDestroyTransition();
	
private:
	// Scene Component(Root)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeometryCollection", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneComponent;
	
	// Geometry Collection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeometryCollection", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeometryCollectionComponent> GeometryCollection;

	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BlockCollision;
	
	FTimerHandle DestroyTimerHandle;
	float DestroyElapsedTime = 0.0f;
	
};
