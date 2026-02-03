#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GollemCharacter.generated.h"

class UPostProcessComponent;
class UHealthComponent;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class USkullyCameraComponent;
class USpringArmComponent;
class UBoxComponent;

UCLASS(Abstract)
class MYSKULLY_API AGollemCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGollemCharacter();
	
	FORCEINLINE bool GetOnClayMound() const { return bOnClayMound; }
	FORCEINLINE void SetOnClayMound(bool Value) { bOnClayMound = Value; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void PawnClientRestart() override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	
	// Components
	void ResolveOptionalComponents();
	
	// Input Mapping Context
	void ApplyInputMappingContext();
	void RemoveInputMappingContext();
	
	// Jump
	void StartFallingMonitor();
	void StopFallingMonitor();
	void CheckFallingApex();
	
	//Spawn
	virtual void StartSpawnFrontCamera();
	virtual void EndSpawnFrontCamera();
	virtual void BeginSpawnFrontCameraReturn();
	virtual void TickSpawnFrontCameraReturn();
	
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gollem|Collision")
	TObjectPtr<UBoxComponent> InteractionBox = nullptr;
	
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gollem|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gollem|Camera")
	TObjectPtr<USkullyCameraComponent> FollowCamera = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UBoxComponent* CameraBoxComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UPostProcessComponent* PostProcessComponent;
	
	// Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UHealthComponent> HealthComponent;
	
	// Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputMappingContext> IMC_Common = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputMappingContext> IMC_FormSpecific = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	int32 IMC_CommonPriority = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	int32 IMC_FormPriority = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Look = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Jump = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Interact = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Primary = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Input")
	TObjectPtr<UInputAction> IA_Secondary = nullptr;
	
	// Jump
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Jump")
	bool bJump = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Jump")
	float GravityScaleGrounded = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Jump")
	float GravityScaleAscending = 2.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gollem|Jump")
	float GravityScaleDescending = 4.0f;
		
	// Input Handler
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void EndJump(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void Primary(const FInputActionValue& Value);
	void Secondary(const FInputActionValue& Value);
	
	// Action
	UFUNCTION(BlueprintNativeEvent, Category = "Gollem|Action")
	void InteractAction();
	virtual void InteractAction_Implementation();
	UFUNCTION(BlueprintNativeEvent, Category = "Gollem|Action")
	void PrimaryAction();
	virtual void PrimaryAction_Implementation();
	UFUNCTION(BlueprintNativeEvent, Category = "Gollem|Action")
	void SecondaryAction();
	virtual void SecondaryAction_Implementation();
	
	bool bInputMappingContextApplied = false;
	
	// Jump
	FTimerHandle FallingCheckTimer;
	bool bDescendingGravityApplied = false;
	
	// Spawn
	bool bSpawnCamActive = false;
	bool bSpawnCamReturning = false;
	FRotator CachedBoomRot;
	FRotator SpawnFrontRot;
	FTimerHandle SpawnCamTimerHandler;
	FTimerHandle SpawnCamReturnTimerHandler;
	float SpawnCamReturnElapsed = 0.0f;
	
	// Puddle
	bool bOnClayMound = false;
};
