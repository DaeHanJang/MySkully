// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SkullyMovementComponent.generated.h"

enum class ESkullyMovementMode
{
	Grounded,
	Falling
};

/**
 * 
 */
UCLASS()
class MYSKULLY_API USkullyMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:
	USkullyMovementComponent();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	// 중력 처리 및 지면 감지
	void ApplyGravity(float DeltaTime);
	void Move(float DeltaTime);
	bool SweepGround(FHitResult& OutHit);
	void CheckGround();

	FVector ConsumeMovementInput();
	
	bool IsWalkable(const FHitResult& Hit) const;
	FVector ComputeSlopeMove(const FVector& Input, const FVector& FloorNormal);
	void SnapToGround(const FHitResult& Hit);
	
	// 저항 계산
	void ApplyFriction(float DeltaTime);
		
protected:
	// 가속도
	FVector Velocity = FVector::ZeroVector;
	
	// 중력값
	UPROPERTY(EditAnywhere, Category = "Gravity")
	float Gravity = 2000.0f;
	
	// 최대 속력
	UPROPERTY(EditAnywhere, Category = "Speed")
	float MaxSpeed = 1200.0f;
	
	// 지면 감지 거리
	UPROPERTY(EditAnywhere, Category = "Ground")
	float GroundCheckDistance = 5.0f;
	
	// 마찰력
	UPROPERTY(EditAnywhere, Category = "Ground|Friction")
	float GroundFriction = 1000.0f;
	
	// 공기 저항력
	UPROPERTY(EditAnywhere, Category = "Air|Friction")
	float AirFriction = 20.0f;
	
	FHitResult CurrentFloorHit;
	
	UPROPERTY(EditAnywhere, Category = "Slope")
	float MaxSlopeAngle = 45.0f;
	
	UPROPERTY(EditAnywhere, Category = "Slope")
	float MaxStepHeight = 40.0f;
	
	FVector CachedFloorNormal = FVector::UpVector;
	
	// 움직임 상태값
	ESkullyMovementMode MovementMode = ESkullyMovementMode::Falling;
	
	UPROPERTY(EditAnywhere, Category = "Slope")
	float ApexForwardNudge = 1.0f;
	
	FVector LastFloorNormal = FVector::UpVector;
	
	UPROPERTY(EditAnywhere, Category = "Slope")
	float FloorNormalInterpSpeed = 12.0f;
	
private:
	static constexpr float MinProjectedMoveSq = 1.0f; // 투영 후 최소 이동량
};
