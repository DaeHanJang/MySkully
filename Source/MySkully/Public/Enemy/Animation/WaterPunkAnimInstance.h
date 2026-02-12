#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WaterPunkAnimInstance.generated.h"

UCLASS()
class MYSKULLY_API UWaterPunkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	FORCEINLINE bool GetWakeUp() const { return bWakeUp; }
	FORCEINLINE void SetWakeUp(const bool Value) { bWakeUp = Value; }
	FORCEINLINE bool GetSee() const { return bSee; }
	FORCEINLINE void SetSee(const bool Value) { bSee = Value; }
	FORCEINLINE bool GetChase() const { return bChase; }
	FORCEINLINE void SetChase(const bool Value) { bChase = Value; }
	FORCEINLINE bool GetHit() const { return bHit; }
	FORCEINLINE void SetHit(const bool Value) { bHit = Value; }
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float SpeedRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bWakeUp = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase")
	bool bSee = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase")
	bool bChase = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bHit = false;
	
};
