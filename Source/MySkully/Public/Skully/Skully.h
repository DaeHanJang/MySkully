#pragma once

#include "CoreMinimal.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "GameFramework/Pawn.h"
#include "Skully.generated.h"

class AGollemCharacter;
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
	FORCEINLINE const FVector& GetClayMoundSurfaceLocation() const { return ClayMountSurfaceLocation; }
	FORCEINLINE void SetClayMoundSurfaceLocation(const FVector& Location) { ClayMountSurfaceLocation = Location;}
	
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Health Interface
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	virtual void OnTakeHealth_Implementation() override;

	// Camera Box Component
	UFUNCTION()
	virtual void OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	virtual void OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	// Camera
	void UpdateFOVBySpeed(float DeltaTime, float Speed, FVector Dir);
	
	// Input
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void StopInteract(const FInputActionValue& Value);
	void TransformStrongGollem(const FInputActionValue& Value);
	void TransformSwiftGollem(const FInputActionValue& Value);
	
	// Puddle
	void UpdateClayMoundTransition();
	
	// Transform
	void TransformToGollem(TSubclassOf<AGollemCharacter> GollemClass);
	
public:
	// Initialize
	void InitState();
	// Hide
	void HideSkully(bool bNoCollision = true, bool bMesh = false);
	// Show
	void ShowSkully();
	// Set Skully_clay Scale
	void SetSkully_ClayScale(float Scale);
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess="true"))
	USphereComponent* SphereComponent;

	// Arrow
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow", meta = (AllowPrivateAccess="true"))
	UArrowComponent* ArrowComponent;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	USceneComponent* MeshPivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
	USkeletalMeshComponent* Skully_Bone;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess="true"))
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
	
	// Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess="true"))
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* TransformStrongAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess="true"))
	UInputAction* TransformSwiftAction;

	// Transform
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform", meta = (AllowPrivateAccess="true"))
	TSubclassOf<AGollemCharacter> StrongGollemClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform", meta = (AllowPrivateAccess="true"))
	TSubclassOf<AGollemCharacter> SwiftGollemClass;
	UPROPERTY()
	TObjectPtr<AGollemCharacter> CurrentGollem;
	
	
	// Cache
	TEnumAsByte<ECollisionEnabled::Type> CachedSphereCollision; // 콜리전 상태 저장 
	FName CachedCollisionProfileName; // 콜리전 프로파일 저장
	
	// Health
	FTimerHandle HealTimerHandle; // 힐 타이머
	
	// Puddle
	bool bOnClayMound = false; // 웅덩이 위에 있는지=웅덩이와 상호작용할 수 있는 위치 인지
	bool bIsInClayMoundInteraction = false; // 웅덩이에서 상호작용 중인지
	bool bTransitioningClayMound = false; // 웅덩이에서 연출 중인지
	bool bClayMoundSubmerged = false; // 웅덩이 잠수/부상 상태
	bool bClayBaseLocked = false;
	FVector ClayMountSurfaceLocation = FVector::ZeroVector; // 웅덩이 표면 위치
	FTimerHandle ClayTransitionTimerHandle; // 웅덩이 연출 타이머
	float ClayAlpha = 0.0f; // 웅덩이 연출 보간값
	
	// Transform
	bool bCanTransform = false; // 변신 가능 플래그
};
