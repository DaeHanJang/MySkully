#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClayMount.generated.h"

class UBoxComponent;

UCLASS()
class MYSKULLY_API AClayMount : public AActor
{
	GENERATED_BODY()
	
public:	
	AClayMount();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
								   int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// Mest
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	UStaticMeshComponent* StaticMesh;
	
	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	UBoxComponent* BoxComponent;
	
	// Save
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	uint8 SaveIndex;
};
