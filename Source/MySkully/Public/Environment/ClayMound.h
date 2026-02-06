#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClayMound.generated.h"

class UBoxComponent;

UCLASS()
class MYSKULLY_API AClayMound : public AActor
{
	GENERATED_BODY()
	
public:
	AClayMound();

protected:	
	UFUNCTION()
	void OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// Mesh(Root)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> ClayMoundMesh;
	
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> InteractionCollision;
	
	// Pivot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pivot", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> BlockerPivot;
	
	// Save
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	uint8 SavePriority = 0;
	
};
