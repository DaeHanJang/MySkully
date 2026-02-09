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
	void ApplyPunchAt(const FVector& WorldPos, const float Strain = 1e10f, const float VelocityMag = 50.0f);

private:
	// Scene Component(Root)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeometryCollection", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneComponent;
	
	// Geometry Collection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeometryCollection", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeometryCollectionComponent> GeometryCollection;

	// Block Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BlockCollision;
	
	// Overlap Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> OverlapCollision;
	
};
