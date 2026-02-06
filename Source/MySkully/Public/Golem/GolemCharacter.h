#pragma once

#include "CoreMinimal.h"
#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveInterface.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "GameFramework/Character.h"
#include "GolemCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UClayMoundReactiveComponent;
class UHealthComponent;
class UBoxComponent;
class USkullyCameraComponent;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(Abstract)
class MYSKULLY_API AGolemCharacter : public ACharacter, public IHealthInterface, public IClayMoundReactiveInterface
{
	GENERATED_BODY()

public:
	AGolemCharacter();
	
	// Input
	virtual void Primary(const FInputActionValue& Value);
	virtual void Secondary(const FInputActionValue& Value);
	
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	virtual void OnTakeHealth_Implementation() override;
	
	// ClayMoundReactive Interface
	virtual void OnEnterClayMound_Implementation() override;
	virtual void OnExitClayMound_Implementation() override;
	
	// Input
	virtual void Move(const FInputActionValue& Value);
	virtual void Look(const FInputActionValue& Value);
	virtual void StartJump(const FInputActionValue& Value);
	virtual void StopJump(const FInputActionValue& Value);
	virtual void Dismount(const FInputActionValue& Value);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void DismountAction();
	virtual void DismountAction_Implementation();
	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void PrimaryAction();
	virtual void PrimaryAction_Implementation();
	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void SecondaryAction();
	virtual void SecondaryAction_Implementation();
	
private:
	// Jump
	void CheckFallingApex();
	
	// Spawn
	void PlaySpawnCameraSequence();
	void UpdateSpawnCameraSequence();

protected:
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USkullyCameraComponent> FollowCamera = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> FollowCameraCollision;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float DefaultFOV = 90.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float MaxFOV = 110.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float FOVInterpSpeed = 10.0f;
	
	bool bIsPlayingCameraSequence = false;
	FRotator CachedCameraBoomRotation;
	
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> InteractionBox = nullptr;
	
	// Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UHealthComponent> HealthComponent;
	
	// ClayMound
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClayMound", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UClayMoundReactiveComponent> ClayMoundReactiveComponent;
	
	// Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputMappingContext> InputMappingContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> MoveInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> LookInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> JumpInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> DismountInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> PrimaryInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> SecondaryInputAction;
	
	//Jump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess="true"))
	float  GravityScaleAscending = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (AllowPrivateAccess="true"))
	float GravityScaleDescending = 4.0f;
	
	FTimerHandle FallingTimerHandle;
	bool bDescending = false;
	
private:
	// Spawn
	FTimerHandle SpawnCameraSequenceTimerHandle;
	FRotator SpawnCameraBoomRotation;
	float SpawnCameraBoomRotationSpeed;
	
};
