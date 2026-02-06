#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "SkullyCameraComponent.generated.h"

class UBoxComponent;

UCLASS()
class MYSKULLY_API USkullyCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	USkullyCameraComponent();
	
	FORCEINLINE UBoxComponent* GetCameraCollision() const { return CameraCollision;}
	FORCEINLINE void SetCameraCollision(UBoxComponent* NewBoxCollision) { CameraCollision = NewBoxCollision;}
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> CameraCollision;
	
};
