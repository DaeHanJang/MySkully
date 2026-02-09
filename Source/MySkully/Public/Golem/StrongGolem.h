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
	
	FORCEINLINE UAnimMontage* GetSlamEndMontage() { return SlamEndMontage; }
	FORCEINLINE void SetPunch(const bool Value) { bPunch = Value; }
	FORCEINLINE void SetSlam(const bool Value) { bSlam = Value; }
	FORCEINLINE bool GetSlamEnding() const { return bSlamEnding; }
	FORCEINLINE void SetSlamEnding(const bool Value) { bSlamEnding = Value; }
	FORCEINLINE void SetSlamPower(const float Value) { SlamPower = Value; }
	
	virtual void Eat() override;
		
protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// Input
	virtual void DismountAction_Implementation() override;
	virtual void DespawnAction_Implementation() override;
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> EatMontage;
	
	bool bPunch = false;
	bool bSlam = false;
	bool bSlamEnding = false;
	
	// Slam
	float SlamPower = 0.0f;
	
};
