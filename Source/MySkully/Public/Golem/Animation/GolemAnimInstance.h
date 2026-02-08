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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (AllowPrivateAccess = "true"))
	float SpeedRadio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (AllowPrivateAccess = "true"))
	bool bJump = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (AllowPrivateAccess = "true"))
	bool bFalling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismount", meta = (AllowPrivateAccess = "true"))
	bool bDismount = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn", meta = (AllowPrivateAccess = "true"))
	bool bDespawn = false;
	
};
