// Fill out your copyright notice in the Description page of Project Settings.


#include "Skully/SkullyMovementComponent.h"

#include "Components/SphereComponent.h"

USkullyMovementComponent::USkullyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USkullyMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (UpdatedComponent == nullptr || ShouldSkipUpdate(DeltaTime) == true)
	{
		return;
	}
	
	ApplyGravity(DeltaTime);
	ApplyFriction(DeltaTime);
	Move(DeltaTime);
	CheckGround();
}

// 중력 적용
void USkullyMovementComponent::ApplyGravity(float DeltaTime)
{
	// 떨어지는 상태이면 추락
	if (MovementMode == ESkullyMovementMode::Falling)
	{
		Velocity.Z -= Gravity * DeltaTime;
	}
	else
	{
		Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
	}
}

// 이동 처리
void USkullyMovementComponent::Move(float DeltaTime)
{
	const FVector Input = ConsumeMovementInput();

	if (Input.IsNearlyZero() == false)
	{
		const FVector InputDir = Input.GetSafeNormal();
		
		Velocity.X = InputDir.X * MaxSpeed;
		Velocity.Y = InputDir.Y * MaxSpeed;
	}
	
	FVector MoveDelta = Velocity * DeltaTime;
	
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		const FVector InputDir = Input.GetSafeNormal();
		
		const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
		
		const bool bUnstableFloor = CachedFloorNormal.Z < 0.99f || NormalDot < 0.95f || CurrentFloorHit.FaceIndex == INDEX_NONE;
		
		FVector AdjustedMove;
		
		if (bUnstableFloor == true)
		{
			AdjustedMove = FVector(InputDir.X * MaxSpeed * DeltaTime, InputDir.Y * MaxSpeed * DeltaTime, 0.0f);
		}
		else
		{
			AdjustedMove = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);
			
			if (AdjustedMove.SizeSquared() < MinProjectedMoveSq)
			{
				AdjustedMove = FVector(InputDir.X * MaxSpeed * DeltaTime, InputDir.Y * MaxSpeed * DeltaTime, 0.0f);
			}
		}
		
		MoveDelta = AdjustedMove;
	}
		
	FHitResult Hit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	
	if (Hit.bBlockingHit == true)
	{
		SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);
		
		if (MovementMode == ESkullyMovementMode::Grounded)
		{
			FVector FloorSlide = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);
			
			if (FloorSlide.IsNearlyZero() == false)
			{
				FHitResult FloorHit;
				SafeMoveUpdatedComponent(FloorSlide * 0.5f, UpdatedComponent->GetComponentQuat(), true, FloorHit);
			}
		}
	}
}

// 바닥 감지
bool USkullyMovementComponent::SweepGround(FHitResult& OutHit)
{
	// 스컬리는 구형이므로 SphereComponent를 기준으로 함
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return false;
	}
	
	const float Radius = Sphere->GetScaledSphereRadius();
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start - FVector::UpVector * (Radius + GroundCheckDistance);
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	
	// Radius 반지름의 구가 Start 지점에서 End 지점까지 부딪치는 것이 인지는 확인
	return GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
}

// 지면에 붙어있는지 상태 처리
void USkullyMovementComponent::CheckGround()
{
	FHitResult Hit;
	const bool bHitGround = SweepGround(Hit);
	
	const float MaxGroundDistance = 8.0f;
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	const float GraceZ = WalkableZ - 0.05f;
	
	// 감지가 성공적으로 됐고 실제 충돌체가 있을 경우
	if (bHitGround == true && Hit.bBlockingHit == true && Hit.Distance <= MaxGroundDistance)
	{
		const float HitZ = Hit.ImpactNormal.Z;
		
		if (HitZ >= WalkableZ)
		{
			CurrentFloorHit = Hit;
			LastFloorNormal = CachedFloorNormal;
			CachedFloorNormal = FMath::VInterpNormalRotationTo(CachedFloorNormal.IsNearlyZero() ? Hit.ImpactNormal : CachedFloorNormal, 
				Hit.ImpactNormal, GetWorld()->GetDeltaSeconds(), FloorNormalInterpSpeed);
			
			const bool bIsSlope = HitZ < 0.999f;
			
			if (bIsSlope == false && MovementMode == ESkullyMovementMode::Falling && Velocity.Z <= 0.0f)
			{
				SnapToGround(Hit);
			}
				
			MovementMode = ESkullyMovementMode::Grounded;
			
			return;
		}
		
		if (MovementMode == ESkullyMovementMode::Grounded && HitZ >= GraceZ)
		{
			CurrentFloorHit = Hit;
			CachedFloorNormal = Hit.ImpactNormal;
			return;
		}
	}
	{
		FHitResult LineHit;
		const FVector Start = UpdatedComponent->GetComponentLocation();
		const FVector End = Start - FVector::UpVector * 12.0f;
		
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		
		if (GetWorld()->LineTraceSingleByChannel(LineHit, Start, End, ECC_Visibility, Params))
		{
			if (LineHit.ImpactNormal.Z >= WalkableZ)
			{
				CurrentFloorHit = LineHit;
				LastFloorNormal = CachedFloorNormal;
				CachedFloorNormal = LineHit.ImpactNormal;
				MovementMode = ESkullyMovementMode::Grounded;
				
				return;
			}
		}
	}
	
	CurrentFloorHit = FHitResult();
	MovementMode = ESkullyMovementMode::Falling;
}

FVector USkullyMovementComponent::ConsumeMovementInput()
{
	// 입력으로 쌓인 벡터(PendingInputVector)을 소비(가져오고 초기화)
	FVector Input = ConsumeInputVector();
	
	// 값이 있다면 정규화
	if (Input.IsNearlyZero() == false)
	{
		Input = Input.GetClampedToMaxSize(1.0f);
	}
	
	return Input;
}

bool USkullyMovementComponent::IsWalkable(const FHitResult& Hit) const
{
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	return Hit.ImpactNormal.Z >= WalkableZ;	
}

FVector USkullyMovementComponent::ComputeSlopeMove(const FVector& Input, const FVector& FloorNormal)
{
	return FVector::VectorPlaneProject(Input, FloorNormal).GetSafeNormal();
}

void USkullyMovementComponent::SnapToGround(const FHitResult& Hit)
{
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return;
	}
	
	const float Radius = Sphere->GetScaledSphereRadius();
	const FVector TargetLocation = Hit.ImpactPoint + Hit.ImpactNormal * Radius;
	
	UpdatedComponent->SetWorldLocation(TargetLocation);
}

// 저항 적용
void USkullyMovementComponent::ApplyFriction(float DeltaTime)
{
	float Friction = (MovementMode == ESkullyMovementMode::Grounded) ? GroundFriction : AirFriction;
	
	FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	
	if (HorizontalVelocity.IsNearlyZero() == false)
	{
		// 저항값 계산
		const FVector Decel = -HorizontalVelocity.GetSafeNormal() * Friction * DeltaTime;
		
		if (Decel.SizeSquared() >= HorizontalVelocity.SizeSquared())
		{
			HorizontalVelocity = FVector::ZeroVector;
		}
		else
		{
			HorizontalVelocity += Decel;
		}
		
		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;
	}
}
