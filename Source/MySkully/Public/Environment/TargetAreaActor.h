#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetAreaActor.generated.h"

class USphereComponent;

UCLASS()
class MYSKULLY_API ATargetAreaActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATargetAreaActor();

private:
	UFUNCTION()
	void OnTargetAreaComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> TargetAreaComponent;

};
