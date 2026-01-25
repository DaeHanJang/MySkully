#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "GameFramework/Pawn.h"
#include "Skully.generated.h"

class UPostProcessComponent;
class UHealthComponent;
class UArrowComponent;
class UCameraComponent;
class USpringArmComponent;
class USkullyMovementComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPawnMovementComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class MYSKULLY_API ASkully : public APawn, public IHealthInterface
{
	GENERATED_BODY()

public:
	ASkully();

protected:
	virtual void BeginPlay() override;
	
	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	USphereComponent* SphereComponent;

	// Arrow
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	UArrowComponent* ArrowComponent;
	
	// Mesh Pivot
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pivot", meta = (AllowPrivateAccess="true"))
	USceneComponent* MeshPivot;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	USkeletalMeshComponent* Skully_Bone;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	UStaticMeshComponent* Skully_Clay;
	
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	USpringArmComponent* CameraSpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UPostProcessComponent* PostProcessComponent;
	
	// Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	USkullyMovementComponent* MovementComponent;
	
	// Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	UHealthComponent* HealthComponent;
	
	// Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* LookAction;

protected:
	// Skully Input Method
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
};
