#pragma once

#include "CoreMinimal.h"
#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveInterface.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "GameFramework/Pawn.h"
#include "Skully.generated.h"

class UAIPerceptionStimuliSourceComponent;
class AGolemCharacter;
class UClayMoundReactiveComponent;
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
class MYSKULLY_API ASkully : public APawn, public IHealthInterface, public IClayMoundReactiveInterface
{
	GENERATED_BODY()
	
public:
	ASkully();

	FORCEINLINE bool GetCanRide() const { return bCanRide; }
	FORCEINLINE void SetCanRide(const bool Value) { bCanRide = Value; }
	FORCEINLINE AGolemCharacter* GetNearbyGolem() const { return NearbyGolem; }
	FORCEINLINE void SetNearbyGolem(AGolemCharacter* NewGolem) { NearbyGolem = NewGolem; }
	
	// Helper
	void Init() const;
	void HideSkully(const bool bNoCollision = true, const bool bMesh = true) const;
	void ShowSkully(const bool bCollision = true, const bool bMovementComp = true, const bool bMesh = true) const;
	
	// Transform
	void DismountGolem(const float ZOffset);
	void DespawnGolem();
	
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	virtual void OnTakeHealth_Implementation() override;
	
	// ClayMoundReactive Interface
	virtual void OnEnterClayMound_Implementation() override;
	virtual void OnExitClayMound_Implementation() override;
	
private:
	// Camera
	void UpdateCameraFOVFromSpeed(float DeltaTime, float Speed, FVector Dir) const;
	
	// Input
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void StopInteract(const FInputActionValue& Value);
	void TransformStrongGolem(const FInputActionValue& Value);
	void TransformSwiftGolem(const FInputActionValue& Value);
	void GolemInteract(const FInputActionValue& Value);
	
	// ClayMound
	void UpdateClayMoundTransition();
	
	// Transform
	void TransformToGolem(const TSubclassOf<AGolemCharacter> GolemClass, const float ZOffset);
		
	// Interaction Collision
	UFUNCTION()
	void OnSphereComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
private:
	// Collision(Root)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	// Arrow
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UArrowComponent> Direction;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> MeshPivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> BoneMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> ClayMesh;
	
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USkullyCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> FollowCameraCollision;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float DefaultFOV = 90.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float MaxFOV = 110.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess="true"))
	float FOVInterpSpeed = 10.0f;
	
	// Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess="true"))
	TObjectPtr<USkullyMovementComponent> SkullyMovementComponent;
	
	// Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UHealthComponent> HealthComponent;
	
	FTimerHandle HealTimerHandle;
	
	// ClayMound
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClayMound", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UClayMoundReactiveComponent> ClayMoundReactiveComponent;
	
	bool bClayMoundInteraction = false;
	bool bDescendingIntoClayMound = false;
	FTimerHandle ClayMoundTransitionTimerHandle;
	float ClayMoundTransitionAlpha = 0.0f;
	
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
	TObjectPtr<UInputAction> ClayMoundInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> TransformStrongGolemInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> TransformSwiftGolemInputAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> GolemInputAction;

	// Transform
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform", meta = (AllowPrivateAccess="true"))
	TSubclassOf<AGolemCharacter> StrongGolemClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform", meta = (AllowPrivateAccess="true"))
	TSubclassOf<AGolemCharacter> SwiftGolemClass;
	
	UPROPERTY()
	TObjectPtr<AGolemCharacter> CurrentGolem;
	UPROPERTY()
	AGolemCharacter* NearbyGolem;
	bool bCanTransform = false;
	bool bCanRide = false;
	
	// Perception
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionSourceComponent;
	
	// Interaction Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> InteractionCollision;
	
};
