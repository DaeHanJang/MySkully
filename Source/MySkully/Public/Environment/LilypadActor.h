#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LilypadActor.generated.h"

UCLASS()
class MYSKULLY_API ALilypadActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ALilypadActor();

protected:
	virtual void BeginPlay() override;
	
private:
	void UpdateLilypad();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	FTimerHandle HideTimerHandle;
	float HideElapsedTime;
};
