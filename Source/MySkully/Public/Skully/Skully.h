#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "GameFramework/Pawn.h"
#include "Skully.generated.h"

class USkullyTrailComponent;
class USkullyCameraComponent;
class UBoxComponent;
class UPostProcessComponent;
class UHealthComponent;
class UArrowComponent;
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

	FORCEINLINE bool GetOnClayMound() const { return bOnClayMound; }
	FORCEINLINE void SetOnClayMound(bool Value) { bOnClayMound = Value; }
	
protected:
	virtual void BeginPlay() override;
	
	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	virtual void OnTakeHealth_Implementation() override;

	// Camera Box Component
	UFUNCTION()
	virtual void OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
								   int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Camera
	UFUNCTION()
	void UpdateFOVBySpeed(float DeltaTime, float Speed, FVector Dir);
	
	// Skully Input Method
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Heal(const FInputActionValue& Value);
	void StopHeal(const FInputActionValue& Value);
	
public:
	// Initialize
	void InitState();
	// Set Skully_clay Scale
	void SetSkully_ClayScale(float Scale);
	
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
	USkullyCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UBoxComponent* CameraBoxComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	UPostProcessComponent* PostProcessComponent;
	
	// Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	USkullyMovementComponent* SkullyMovementComponent;
	
	//Trail
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	USkullyTrailComponent* SkullyTrailComponent;
	
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* ClayMoundAction;

private:
	bool bOnClayMound = false; // 웅덩이 상호작용 플래그
	FTimerHandle HealTimerHandle; // 힐 타이머 핸들
};
