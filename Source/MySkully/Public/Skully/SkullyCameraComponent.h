#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "SkullyCameraComponent.generated.h"

UCLASS()
class MYSKULLY_API USkullyCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	USkullyCameraComponent();
	
	// 기본 FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV")
	float BaseFOV = 90.0f;
	// 최대 FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV")
	float MaxFOV = 110.0f;
	// FOV 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV")
	float FOVInterpSpeed = 10.0f;
};
