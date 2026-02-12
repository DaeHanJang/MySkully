#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StaticOverlapObject.generated.h"

UCLASS()
class MYSKULLY_API AStaticOverlapObject : public AActor
{
	GENERATED_BODY()
	
public:	
	AStaticOverlapObject();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;
	
};
