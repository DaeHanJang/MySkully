#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GolemAnimInstance.generated.h"

UCLASS()
class MYSKULLY_API UGolemAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	FORCEINLINE void SetDismount(const bool Value) { bDismount = Value; }
	FORCEINLINE void SetDespawn(const bool Value) { bDespawn = Value; }
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float SpeedRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	bool bJump = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	bool bFalling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismount")
	bool bDismount = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn")
	bool bDespawn = false;
	
};
