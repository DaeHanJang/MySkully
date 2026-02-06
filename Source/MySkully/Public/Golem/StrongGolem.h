#pragma once

#include "CoreMinimal.h"
#include "Golem/GolemCharacter.h"
#include "StrongGolem.generated.h"

UCLASS()
class MYSKULLY_API AStrongGolem : public AGolemCharacter
{
	GENERATED_BODY()
	
public:
	AStrongGolem();
	
	FORCEINLINE void SetPunch(const bool Value) { bPunch = Value; }
	FORCEINLINE void SetSlam(const bool Value) { bSlam = Value; }
	FORCEINLINE UAnimMontage* GetSlamEndMontage() { return SlamEndMontage; }
	FORCEINLINE void SetSlamPower(const float Value) { SlamPower = Value; }
	FORCEINLINE bool GetSlamEnding() const { return bSlamEnding; }
	FORCEINLINE void SetSlamEnding(const float Value) { bSlamEnding = Value; }
	
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// Input
	virtual void DismountAction_Implementation() override;
	virtual void PrimaryAction_Implementation() override;
	virtual void SecondaryAction_Implementation() override;
	
private:
	// Input
	void StopSecondary(const FInputActionValue& Value);

private:	
	// Animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> PunchMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> SlamStartMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> SlamEndMontage;
	
	bool bPunch = false;
	bool bSlam = false;
	bool bSlamEnding = false;
	
	// Slam
	float SlamPower = 0.0f;
	
};
