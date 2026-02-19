#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StaticBlockObject.generated.h"

class UBoxComponent;

UCLASS()
class MYSKULLY_API AStaticBlockObject : public AActor
{
	GENERATED_BODY()
	
public:	
	AStaticBlockObject();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UBoxComponent> CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;

};
