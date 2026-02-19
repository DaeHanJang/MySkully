#pragma once

#include "CoreMinimal.h"
#include "Golem/GolemCharacter.h"
#include "StrongGolem.generated.h"

class UNiagaraSystem;
class USphereComponent;

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
	FORCEINLINE float GetSlamPower() const { return SlamPower; }
	FORCEINLINE void SetSlamPower(const float Value) { UE_LOG(LogTemp, Warning, TEXT("SlamPower: %f"), Value); SlamPower = Value; }
	
	virtual void Eat() override;
	
	// Punch Collision
	void BeginPunchWindow();
	void EndPunchWindow();
	
	// Slam
	void FireSlamBreath();
		
protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	
	// Input
	virtual void DismountAction_Implementation() override;
	virtual void DespawnAction_Implementation() override;
	virtual void PrimaryAction_Implementation() override;
	virtual void SecondaryAction_Implementation() override;
	
	// Punch
	UFUNCTION()
	void OnFistOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
private:
	// Input
	void StopSecondary(const FInputActionValue& Value);
	
	// Slam
	void UpdateSlamBreath();
	void CollectHitActorsWithOcclusionFilter(const FVector& CenterPos, float SphereRadius, TArray<FOverlapResult>& Overlaps);
	bool HasLineOfSlamBreathToActor(const FVector& From, AActor* Target) const;
	
private:
	// Punch Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> PunchCollision;
	
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
	
	// Punch
	TSet<TWeakObjectPtr<AActor>> HitActorsThisPunch;
	
	// Slam
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slam", meta = (AllowPrivateAccess = "true"))
	float MaxSlamPower = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slam", meta = (AllowPrivateAccess = "true"))
	float BaseRadius = 200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slam", meta = (AllowPrivateAccess = "true"))
	float AdditionalRadius = 50.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slam", meta = (AllowPrivateAccess = "true"))
	float SlamBreathStartZOffset = -250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slam", meta = (AllowPrivateAccess = "true"))
	float SlamBreathStartForwardOffset = 100.0f;
	
	float SlamPower = 0.0f;
	TSet<TWeakObjectPtr<AActor>> HitActorsThisSlam;
	FTimerHandle SlamTimerHandle;
	uint8 MaxStep;
	uint8 Step;
	FVector SlamBreathStartLocation;
	FVector SlamBreathStartLocationForwardVector;
	float Range;
	float CurAdditionalPos;
	float Radius;
	
	// Effect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> Effect;
	
};
