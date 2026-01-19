// Fill out your copyright notice in the Description page of Project Settings.


#include "Skully/SkullyMovementComponent.h"

#include "LandscapeGizmoActiveActor.h"
#include "Components/SphereComponent.h"

namespace
{
	bool TryGetDownhillDirFromSamples(UWorld* World, const FVector& Origin, float SampleDist, float TraceDown, AActor* IgnoreActor, FVector& OutDir);
}

USkullyMovementComponent::USkullyMovementComponent()
{
	// 이 컴포넌트는 매 Tick마다 자체 물리(중력/마찰/이동/지면판정)를 처리한다
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

	// 중력 적용(Falling일 때만 Z 하강(아래로 가속))
	ApplyGravity(DeltaTime);
	// 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
	bSlopeSlideAppliedThisFrame = ApplySlopeSlide(DeltaTime);
	// 마찰 적용(XY 감속(XY 속도를 줄여 미끄러짐/관성을 제어))
	ApplyFriction(DeltaTime, bSlopeSlideAppliedThisFrame ? SlidingFriction : GroundFriction);
	// 이동 처리(Sweep 기반 이동 + 충돌 처리(입력 기반 + 경사 투영 + 불안정 바닥 처리))
	Move(DeltaTime);
	// 지면 판정(Sweep + LineTrace로 Grounded/Falling 갱신)
	// Move()가 먼저 움직인 뒤, CheckGround()가 새 위치에서 바닥 상태를 확정
	CheckGround(DeltaTime);
	// 이동 상태값(현재 속력, 방향 등) 갱신
	UpdateMotionState();
}

// 중력 적용
void USkullyMovementComponent::ApplyGravity(float DeltaTime)
{
	// Falling일 때만 중력 가속을 적용해서 낙하
	if (MovementMode == ESkullyMovementMode::Falling)
	{
		Velocity.Z -= Gravity * DeltaTime;
	}
	// Grounded 상태에서는 중력으로 바닥을 파고들지 않도록 Z 속도를 최소 0으로 유지
	else
	{
		const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
		const bool bWalkableFloor = CachedFloorNormal.Z >= WalkableZ;
		Velocity.Z = bWalkableFloor ? -GroundStickDownSpeed : 0.0f;
	}
}

// 경사면 미끄러짐 적용: 플레이어가 가만히 있는데 경사가 있으면 굴러떨어지는 전용 로직
bool USkullyMovementComponent::ApplySlopeSlide(float DeltaTime)
{
	// 기본 전제: Grounded + 입력 없음일 때만 정지 후 굴러떨어짐을 평가한다.
	if (MovementMode != ESkullyMovementMode::Grounded || GetPendingInputVector().IsNearlyZero() == false)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	// 불안정 바닥에서는 CachedFloorNormal이 튀는 프레임이 있으므로, 슬라이드에 사용할 노멀을 안정화하여 UseNormal에 적용
	const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
	const bool bUnstableFloor = CachedFloorNormal.Z < UnstableFloorZThreshold || NormalDot < FloorNormalDotEdgeThreshold;
	FVector UseNormal = bUnstableFloor ? LastFloorNormal : CachedFloorNormal;
	UseNormal = UseNormal.GetSafeNormal();
	// 중력 벡터를 바닥 평면에 투영하여 미끄러짐 생성
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UseNormal);
	
	// 상각형 경계, 플랫폼 끝, 경사-평지 전환 등으로 노멀이 애매해서 AlongPlane이 거의 0이면
	if (AlongPlane.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		FVector DownhillDir;
		// 주변 바닥 높이를 샘플링해서 진짜 내리막 방향(downhill)을 추정
		if (UseNormal.Z < MinSlopeForSamplesZ && 
			TryGetDownhillDirFromSamples(GetWorld(), UpdatedComponent->GetComponentLocation(), DownhillSampleDistance, GroundLineTraceDistance + 50.0f, GetOwner(), DownhillDir))
		{
			AlongPlane = DownhillDir * Gravity;
		}
		else
		{
			bIsSlopeSliding = false;
			return false;
		}
	}
		
	// 슬라이드 가속 스케일 적용
	FVector SlideAccel = AlongPlane * SlopeSlideScale;
	const float SlideAccelMag = SlideAccel.Size();
	
	if (SlideAccelMag <= KINDA_SMALL_NUMBER)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	const FVector SlideDir = SlideAccel.GetSafeNormal();
	
	// 슬라이딩 종료(히스테리시스): 슬라이딩 상태일 때 가속이 너무 작으면 시작 안함
	const float StopThreshold = StaticFrictionAccel * 0.5f;
	if (bIsSlopeSliding == true && SlideAccelMag < StopThreshold)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 시작(Static friction): 아직 슬라이딩 중이 아니면 정지 마찰을 이겨야 시작
	bool bStartedSlidingThisFrame = false;
	if (bIsSlopeSliding == false)
	{
		if (SlideAccelMag <= StaticFrictionAccel)
		{
			// 정지 마찰이 이기면 슬라이드 시작을 허용하지 않는다.
			// 여기서 Velocity를 0으로 강제하면(특히 엣지/경계에서 노멀이 흔들릴 때)
			// 속도 리셋 -> 다음 프레임 보정 이동같은 에너지 폭발이 생길 수 있어 건드리지 않는다.	
			return false;
		}
		
		// 여기까지 왔으면 미끄러지기 시작
		bIsSlopeSliding = true;
		bStartedSlidingThisFrame = true;
	}
	
	// 슬라이드 적용 전(이번 프레임 시작 시점)의 수평 속도
	const FVector Vel2DBefore(Velocity.X, Velocity.Y, 0.f);
	const bool bWasNearlyZero = (Vel2DBefore.SizeSquared() < FMath::Square(1.0f));
	
	// 유지(Kinetic friction): 정지 마찰을 이긴 만큼만 가속(부드러운 시작)
	Velocity.X += SlideDir.X * SlideAccelMag * DeltaTime;
	Velocity.Y += SlideDir.Y * SlideAccelMag * DeltaTime;
	
	// 아주 작은 속도에서 한 프레임 멈칫하는 현상 방지: 최소 시작 속도 보장
	if (bStartedSlidingThisFrame == true && bWasNearlyZero == true)
	{
		FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		const float Speed2D = Velocity2D.Size();
		
		if (Speed2D > KINDA_SMALL_NUMBER && Speed2D < MinSlopeSlideStartSpeed)
		{
			Velocity2D = SlideDir * MinSlopeSlideStartSpeed;
			Velocity.X = Velocity2D.X;
			Velocity.Y = Velocity2D.Y;
		}
	}
	
	// 속도 제한
	FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
	Velocity2D = Velocity2D.GetClampedToMaxSize(MaxSlopeSlideSpeed);
	Velocity.X = Velocity2D.X;
	Velocity.Y = Velocity2D.Y;
	
	return true;
}

// 마찰 적용
void USkullyMovementComponent::ApplyFriction(float DeltaTime, float GroundedFriction)
{
	// Grounded/Air 상태에 따라 마찰 계수 다르게 사용
	float Friction = (MovementMode == ESkullyMovementMode::Grounded) ? GroundedFriction : AirFriction;
	
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);

	if (HorizontalVelocity.IsNearlyZero() == false)
	{
		// 슬라이딩 중에는 고정 감속(브레이킹) 대신 속도 비례 감쇠(댐핑)를 사용한다.
		// 고정 감속은 저속에서 0으로 스냅되며 툭툭 끊김이 생기기 쉽다
		// 댐핑은 속도가 높을수록 더 많이 감쇠되어 자연스러운 종단속도(terminal speed)를 만든다.
		if (MovementMode == ESkullyMovementMode::Grounded && bIsSlopeSliding == true)
		{
			const float Factor = FMath::Clamp(1.0f - SlideDamping * DeltaTime, 0.0f, 1.0f);
			HorizontalVelocity *= Factor;
		}
		else
		{
			// Friction은 초당 감속량처럼 동작
			const FVector Decel = -HorizontalVelocity.GetSafeNormal() * Friction * DeltaTime;

			if (Decel.SizeSquared() >= HorizontalVelocity.SizeSquared())
			{
				HorizontalVelocity = FVector::ZeroVector;
			}
			else
			{
				HorizontalVelocity += Decel;
			}
		}

		// 마찰은 XY에만 적용
		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;
	}
}

// 이동 처리
void USkullyMovementComponent::Move(float DeltaTime)
{
	// 누적 입력 벡터를 소비
	const FVector Input = ConsumeMovementInput();
	// 입력 벡터가 0에 가깝지 않다면 true
	const bool bHasInput = !Input.IsNearlyZero();
	// 입력 벡터의 방향
	const FVector InputDir = bHasInput ? Input.GetSafeNormal() : FVector::ZeroVector;
	// 시작 위치
	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	
	// 목표 힘
	FVector TargetVelocity2D = FVector::ZeroVector;
	if (bHasInput == true)
	{
		TargetVelocity2D = FVector(InputDir.X, InputDir.Y, 0.0f) * MaxSpeed;
	}
	
	// 벽 투영(벽 제약)
	if (WallDetachTimeLeft > 0.0f && LastWallNormal.IsNearlyZero() == false)
	{
		WallDetachTimeLeft -= DeltaTime;
		TargetVelocity2D = FVector::VectorPlaneProject(TargetVelocity2D, LastWallNormal);
	}
	
	// 경사면 윗 방향 속도 감소
	if (MovementMode == ESkullyMovementMode::Grounded && bHasInput == true)
	{
		// 바닥 노멀
		const FVector FloorN = CachedFloorNormal.GetSafeNormal();
		
		// 내리막/오르막 방향
		const FVector DownSlopeDir = FVector::VectorPlaneProject(FVector(0.0f, 0.0f, -1.0f), FloorN).GetSafeNormal();
		const FVector UpSlopeDir = -DownSlopeDir;
		
		// 입력이 오르막을 얼마나 향하나(0~1)
		const float Uphill = FMath::Clamp(FVector::DotProduct(InputDir, UpSlopeDir), 0.0f, 1.0f);
		
		// FlatGroundZThreshold - WalkableZ: 평지 취급 경계부터 최대 경사까지를 0~1로 맵핑 
		// FlatGroundZThreshold - WalkableZ가 너무 작으면(FlatGroundZThreshold가 WalkableZ에 너무 가까우면) 
		// 조금만 변해도 SlopeSteep이 확 커져서 또 경계가 생김
		const float Denom = FMath::Max(FlatGroundZThreshold - WalkableZ, 0.01f);
		// 경사가 얼마나 가파른가(0~1): FlatGroundZThreshold 근처면 0, 가팔라질수록 1
		float SlopeSteep = FMath::Clamp((FlatGroundZThreshold - FloorN.Z) / Denom, 0.0f, 1.0f);
		// 0~1을 부드럽게: 0 근처는 더 완만, 중간부터 서서히 증가 
		SlopeSteep = SlopeSteep * SlopeSteep * (3.0f - 2.0f * SlopeSteep);
		
		const float SpeedScale = 1.0f - (Uphill * SlopeSteep * UphillSpeedPenalty);
		TargetVelocity2D *= FMath::Clamp(SpeedScale, MinUphillSpeedScale, 1.0f);
	}
	
	// 현재 힘
	FVector CurrentVelocity2D(Velocity.X, Velocity.Y, 0.0f);
	if (TargetVelocity2D.IsNearlyZero() == false)
	{
		// 가속
		CurrentVelocity2D = FMath::VInterpConstantTo(CurrentVelocity2D, TargetVelocity2D, DeltaTime, Acceleration);
	}
	
	CurrentVelocity2D = CurrentVelocity2D.GetClampedToMaxSize(MaxSpeed);
	Velocity.X = CurrentVelocity2D.X;
	Velocity.Y = CurrentVelocity2D.Y;

	// 이번 프레임 이동량 계산
	FVector MoveDelta = Velocity * DeltaTime;

	// Grounded일 때 경게/경사 이동 보정
	// 경계/꼭지점에서 PlaneProject가 이동 벡터를 0으로 만들 수 있으므로, 이때는 투영 대신 입력 기반 이동으로 탈출
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		// 현재 X/Y축 힘
		const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		// 현재 힘의 크기
		const float Speed2D = Velocity2D.Size();
		// 현재 힘의 방향
		const FVector VelocityDir2D = (Speed2D > KINDA_SMALL_NUMBER) ? (Velocity2D / Speed2D) : FVector::ZeroVector;
		
		// 현재 바닥의 노멀과 이전 프레임 바닥의 노멀의 내적: 현재 바닥 노멀과 이전 프레임 바닥 노멀의 변화량
		const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
		// 안정적인 바닥인지 체크: 바닥 노멀을 믿기 어려운 상황(경계/꼭짓점/급격한 노멀 변동)을 감지하는 플래그
		// 조건1: 바닥이 충분히 평평하지 않거나(경사), 노멀 자체가 수직인 특이 케이스
		// 조건2: 노멀 변화량이 급격히 달라진 경우(면-면 경계/엣지)
		const bool bUnstableFloor = CachedFloorNormal.Z < UnstableFloorZThreshold || 
			NormalDot < FloorNormalDotEdgeThreshold;
		// 보정 이동
		FVector AdjustedMove;
		// 엣지/급변 노멀 체크
		const bool bVeryEdge = NormalDot < FloorNormalDotEdgeThreshold;
		// 슬라이드로 이미 자연스러운 속도가 만들어진 프레임에는
		// 투영 붕괴(엣지/경계)에서의 최소 이동 보장(MinProjectedMoveCm)이 에너지를 인위적으로 주입할 수 있다.
		// 입력이 있는 경우엔 조작감/정지 방지가 중요하니 최소 이동 보장을 허용
		// 입력이 없고(정지 후 굴러떨어짐) 슬라이드가 적용된 프레임에는 최소 이동 보장을 끈다
		const bool bAllowMinMoveGuarantee = bHasInput == true && bVeryEdge == false;
		
		// 불안정 바닥이면
		if (bUnstableFloor == true)
		{
			const FVector FallbackDir = (bHasInput ? InputDir : VelocityDir2D);
			if (FallbackDir.IsNearlyZero() == false)
			{
				const float CurrentSpeed = CurrentVelocity2D.Size();
				// 입력 기반으로 강제 이동
				AdjustedMove = FVector(FallbackDir.X * CurrentSpeed * DeltaTime, FallbackDir.Y * CurrentSpeed * DeltaTime, 0.0f);
			}
			else
			{
				// 방향이 없으면 최소한 투영 이동을 시도(완전 정지 + 애매한 히트 프레임)
				AdjustedMove = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);				
			}
		}
		// 안정 바닥이면
		else
		{
			// 바닥 평면으로 투영
			AdjustedMove = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);

			// 투영 결과가 너무 작으면
			if (AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
			{
				const FVector FallbackDir = (bHasInput ? InputDir : VelocityDir2D);
				const float CurrentSpeed = CurrentVelocity2D.Size();
				// 투영이 붕괴하면(엣지/경계) fallback을 사용하되, MaxSpeed로 덮지 않고 현재 속도를 사용
				AdjustedMove = FVector(FallbackDir.X * CurrentSpeed * DeltaTime, FallbackDir.Y * CurrentSpeed * DeltaTime, 0.0f);
			
				// 완전 처음 가속 초반에 CurrentSpeed가 너무 작으면 최소 이동 보장
				// (단, 입력이 없는 슬라이드 프레임에는 에너지 주입/워프를 막기 위해 적용하지 않는다)
				if (bAllowMinMoveGuarantee == true && AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
				{
					AdjustedMove = FVector(FallbackDir.X, FallbackDir.Y, 0.0f) * MinProjectedMoveCm;
				}
			}
		}
		
		MoveDelta = AdjustedMove;
	}
	
	if (WallDetachTimeLeft > 0.0f && LastWallNormal.IsNearlyZero() == false)
	{
		MoveDelta = FVector::VectorPlaneProject(MoveDelta, LastWallNormal);
	}
	
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		// 바닥 노멀
		const FVector FloorN = CachedFloorNormal;
		
		// 바닥이 안정적인지
		const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
		const bool bUnstableFloor = CachedFloorNormal.Z < UnstableFloorZThreshold || NormalDot < FloorNormalDotEdgeThreshold;
		// 오르막/가파른 경사에서 적용하고 싶으면 조건을 건다
		const bool bWalkableFloor = FloorN.Z >= WalkableZ;
		const bool bSteepSlope = FloorN.Z < FlatGroundZThreshold; // 평지 제외
		const bool bHasSpeed2D = FVector(Velocity.X, Velocity.Y, 0.0f).SizeSquared() > FMath::Square(10.0f);
		
		if (bUnstableFloor == true && bWalkableFloor == true && bSteepSlope == true && bHasSpeed2D == true)
		{
			// 바닥 방향으로 살짝 눌러준다(바닥 노멀 반대방향)
			// 너무 과하게 눌러시 지형을 파고들 수 있으므로 Min
			const float StickDelta = FMath::Min(GroundStickForce * DeltaTime, 5.0f);
			
			MoveDelta += (-FloorN) * StickDelta;
		}
	}
	
	// Sweep 이동(관통 방지) + Hit 결과를 돌려줌
	FHitResult Hit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

	const bool bIsWallHitNow = Hit.bBlockingHit == true && Hit.Normal.Z < WallNormalZThreshold;
	if (bIsWallHitNow == true)
	{
		LastWallNormal = Hit.Normal;
		WallDetachTimeLeft = WallDetachCooldown;
	}
	
	// 막혔으면
	if (Hit.bBlockingHit == true)
	{
		const float Nz = Hit.Normal.Z;
		const bool bIsWall = Nz < WallNormalZThreshold;
		const bool bUnwalkableSlope = Hit.Normal.Z < WalkableZ && bIsWall == false;
		
		// 면 안으로 파고드는 속도 제거
		const float Into = FVector::DotProduct(Velocity, Hit.Normal);
		if (Into < 0.0f)
		{
			Velocity -= Hit.Normal * Into;
		}
		
		if (bUnwalkableSlope == true)
		{
			if (bHasInput == true)
			{
				if (TryStepUp(MoveDelta, Hit, DeltaTime) == true)
				{
					return;
				}
			}
			
			// 지금 프레임에서 의도한 2D 이동(조작 방향 기반)
			// (MoveDelta는 이미 바닥 투영/보정이 들어가 잇어서 steep 면에서는 슬라이드가 0으로 죽기 쉬움)
			const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
			const float Speed2D = Velocity2D.Size();
			const FVector DesiredDir2D = bHasInput ? FVector(InputDir.X, InputDir.Y, 0.0f) 
				: (Speed2D > KINDA_SMALL_NUMBER ? Velocity2D / Speed2D : FVector::ZeroVector);
			FVector DesiredDelta = DesiredDir2D * Speed2D * DeltaTime;
			
			// 면 안쪽 성분 제거(벽 슬라이드처럼)
			FVector Slide = FVector::VectorPlaneProject(DesiredDelta, Hit.Normal);
			
			// 오르막(상승) 성분만 제거
			FVector UphillDir = FVector::VectorPlaneProject(FVector::UpVector, Hit.Normal);
			if (UphillDir.IsNearlyZero() == false)
			{
				UphillDir = UphillDir.GetSafeNormal();
				// Slide가 오르막(상승)으로 향하는 성분만 제거
				const float UpAmount = FVector::DotProduct(Slide, UphillDir);;
				if (UpAmount > 0.0f)
				{
					Slide -= UphillDir * UpAmount;
				}
			}
			
			// 슬라이드가 너무 작으면(거의 0) 그냥 벽 슬라이드 함수로 fallback
			// (엣지/코너/특이 케이스에서 완전 정지가 나오는 걸 방지)
			if (Slide.SizeSquared() < FMath::Square(0.5f))
			{
				SlideAlongSurface(DesiredDelta, 1.0f - Hit.Time, Hit.Normal, Hit);
				
				return;
			}
			
			// 남은 시간만큼 이동
			const float RemainingTime = 1.0f - Hit.Time;
			if (RemainingTime > KINDA_SMALL_NUMBER)
			{
				FHitResult SlideHit;
				SafeMoveUpdatedComponent(Slide * RemainingTime, UpdatedComponent->GetComponentQuat(), true, SlideHit);
			}
			
			// 기존 SlideAlongSurface() 타지 않게 차단
			return;
		}
		
		// 벽을 타고 미끄러짐 시도
		SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);

		const bool bIsWallHit = Hit.Normal.Z < WallNormalZThreshold;
		
		if (bIsWallHit == true)
		{
			// 벽 안으로 들어가려는 속도 성분 제거: 벽에 미끄러지다 벽에서 완진히 미끄러진 후 앞으로 폭발하는 현상 제거
			const float IntoWall = FVector::DotProduct(Velocity, Hit.Normal);
			if (IntoWall < 0.0f)
			{
				Velocity -= Hit.Normal * IntoWall;
			}
			// 벽에 박는 입력이면 추가로 감속/정지
			const float PushIntoWall = FVector::DotProduct(InputDir, Hit.Normal);
			if (bHasInput && PushIntoWall < -0.2f)
			{
				Velocity.X *= WallImpactSpeedDamping;
				Velocity.Y *= WallImpactSpeedDamping;
			}
		}
		
		// Grounded면 한 번 더 바닥 기준 재투영 이동을 짧게 시도
		// 경계(Edge)에서 막힐 때 Hit.Normal은 벽도 아니고 바닥도 아닌 애매한 노멀일 수 있으므로 1회 슬라이드로는 이동이 소실되기 쉬움
		// 따라서 Edge/Corner에서는 1회 슬라이드로 이동이 0이 될 수 있어서 2-pass로 재시도
		// 1차: 벽 노멀 기준 슬라이드
		// 2차: 바닥 노멀 기준 재투영 짧은 이동 시도
		if (MovementMode == ESkullyMovementMode::Grounded)
		{
			// 경사 슬라이드가 적용된 프레임엔 2-pass(바닥 재투영)로 이동을 한 번 더 시도하면
			// 엣지/경계에서 한 프레임에 이동이 두 번 발생하며 이동 폭발이 생길 수 있으므로 중단
			if (bSlopeSlideAppliedThisFrame == true)
			{
				return;
			}
			
			FVector FloorSlide = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);

			if (FloorSlide.IsNearlyZero() == false)
			{
				FHitResult FloorHit;
				// FloorSlide * 0.5f는 과도한 재시도로 튀는 것을 방지하기 위한 안전 스텝
				SafeMoveUpdatedComponent(FloorSlide * 0.5f, UpdatedComponent->GetComponentQuat(), true, FloorHit);
			}
		}
	}
	
	const FVector EndLocation = UpdatedComponent->GetComponentLocation();
	const FVector ActualDelta = EndLocation - StartLocation;
	
	if (DeltaTime > KINDA_SMALL_NUMBER)
	{
		const bool bHadBlockingHit = Hit.bBlockingHit;
		const bool bIsWallHit = bHadBlockingHit == true && Hit.Normal.Z < WallNormalZThreshold;
		
		if (bIsWallHit == true)
		{
			const FVector ActualVelocity2D = FVector(ActualDelta.X, ActualDelta.Y, 0.0f) / DeltaTime;
			Velocity.X = ActualVelocity2D.X;
			Velocity.Y = ActualVelocity2D.Y;			
		}
	}
}

// 계단 이동 처리
bool USkullyMovementComponent::TryStepUp(const FVector& MoveDelta, const FHitResult& Hit, float DeltaTime)
{
	if (MovementMode != ESkullyMovementMode::Grounded)
	{
		return false;
	}
	
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return false;
	}
	
	// 벽이 너무 완만하면 step이 아니라 경사로 취급할 수도 잇으니 step은 거의 벽일 때만 시도
	if (Hit.Normal.Z > 0.2f)
	{
		return false;
	}
	
	const float Radius = Sphere->GetScaledSphereRadius();
	const float StepUpHeight = MaxStepHeight;
	
	if (StepUpHeight <= 0.0f)
	{
		return false;
	}
	
	// 위로 올려서 공간이 있는지 체크
	FHitResult UpHit;
	SafeMoveUpdatedComponent(FVector(0.0f, 0.0f, StepUpHeight), UpdatedComponent->GetComponentQuat(), true, UpHit);
	if (UpHit.bBlockingHit == true)
	{
		// 위로 못 올라가면 실패 -> 원위치 복구
		SafeMoveUpdatedComponent(FVector(0.0f, 0.0f, -StepUpHeight), UpdatedComponent->GetComponentQuat(), true, UpHit);
		return false;
	}
	
	// 앞으로 조금 이동(턱 위로)
	const FVector ForwardDelta = FVector(MoveDelta.X, MoveDelta.Y, 0.0f);
	FHitResult ForwardHit;
	SafeMoveUpdatedComponent(ForwardDelta, UpdatedComponent->GetComponentQuat(), true, ForwardHit);
	
	if (ForwardHit.bBlockingHit == true)
	{
		// 앞으로도 막히면 실패 -> 되돌림
		SafeMoveUpdatedComponent(-ForwardDelta, UpdatedComponent->GetComponentQuat(), true, ForwardHit);
		SafeMoveUpdatedComponent(FVector(0.0f, 0.0f, -StepUpHeight), UpdatedComponent->GetComponentQuat(), true, ForwardHit);
		return false;
	}
	
	// 아래로 내려서 착지
	FHitResult DownHit;
	SafeMoveUpdatedComponent(FVector(0.0f, 0.0f, -StepUpHeight - Radius - 2.0f), UpdatedComponent->GetComponentQuat(), true, DownHit);
	
	if (DownHit.bBlockingHit == false)
	{
		return false;
	}
	
	return true;
}

// PawnMovementComponent의 입력 누적 벡터를 가져오고 초기화
FVector USkullyMovementComponent::ConsumeMovementInput()
{
	// 입력으로 쌓인 벡터(PendingInputVector)을 소비(가져오고 초기화)
	FVector Input = ConsumeInputVector();

	if (Input.IsNearlyZero() == false)
	{
		// 크기 정규화(크기를 1로 클램프하여 대각선 입력이 과속이 되지 않게 함)
		Input = Input.GetClampedToMaxSize(1.0f);
	}

	return Input;
}

// 지면 판정: 바닥 상태를 판정(+ 경사각 조건 + 흔들림 방지(깜빡임 방지) + 보조 바닥 확인)
void USkullyMovementComponent::CheckGround(float DeltaTime)
{
	const ESkullyMovementMode PrevMode = MovementMode;
	
	// 바닥 감지
	FHitResult Hit;
	const bool bHitGround = SweepGround(Hit);
	// 허용 경사각
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	// Walkable 판정이 살짝 흔들려도(노멀 튐) Grounded 상태를 유지하는 관용값
	// 없으면 경계면/경계/꼭짓점에서 노멀 값이 할 프레임씩 튀면서 Grounded <-> Falling이 깜빡거림(Ground jitter)
	// 그 결과 이동이 끊기고 미세한 떨림, 툭툭 끊기는 느낌이 남
	const float GraceZ = WalkableZ - GroundGraceZOffset;

	bool bShouldBeGrounded = false;
	bool bWalkable = false;
	
	// 바닥 감지 성공 && Sweep 충돌 성공 && Sweep 충돌 거리가 MaxGroundDistance보다 작으면 유효 바닥 후보
	if (bHitGround == true && Hit.bBlockingHit == true && Hit.Distance <= MaxGroundDistance)
	{
		// 바닥 노멀의 Z(1: 완전한 평지, 작아질수록: 가파른 평지)
		const float HitZ = Hit.ImpactNormal.Z;
		
		// Sweep에 감지된 바닥의 노멀의 Z값이 허용 경사각(WalkableZ)보다 크다면 지면 판정
		if (HitZ >= WalkableZ)
		{
			bShouldBeGrounded = true;
			bWalkable = true;
		}
		else if (PrevMode == ESkullyMovementMode::Grounded && HitZ >= GraceZ)
		{
			bShouldBeGrounded = true;
			bWalkable = false; // 진짜 Walkable은 아니지만 유지
		}
		
		if (bShouldBeGrounded == true)
		{
			CurrentFloorHit = Hit;
			
			const bool bIsSlope = HitZ < FlatGroundZThreshold;
			
			// 평지에 가까우면: 보간하지 말고 즉시 업벡터로 스냅(경사 잔상 제거)
			if (bIsSlope == false)
			{
				LastFloorNormal = FVector::UpVector;
				CachedFloorNormal = FVector::UpVector;
				// 평지에 닿았으면 경사 슬라이딩 상태도 끊어주는 게 안전
				bIsSlopeSliding = false;
			}
			// 평지에 가깝지 않으면
			else
			{
				// 바닥 노멀 캐싱 + 노멀 보간(안정화)
				// 면-면 경계에서 노멀이 튀는 것을 완화
				// Move에서 사용할 바닥 기준 좌표계 역할
				LastFloorNormal = CachedFloorNormal;
				// CachedFloorNormal을 Hit.ImpactNormal로 부드럽게 회전시키는 보간
				// 매 프레임 바닥 노멀이 갑자기 바뀌는(경계/엣지/꼭짓점) 상황에서 튀어버리는 것을 막고 이동 투영이 프레임마다 요동치는 것을 줄임
				CachedFloorNormal = FMath::VInterpNormalRotationTo(
					CachedFloorNormal.IsNearlyZero() ? Hit.ImpactNormal : CachedFloorNormal,
					Hit.ImpactNormal, DeltaTime, FloorNormalInterpSpeed);
			}
			
			MovementMode = ESkullyMovementMode::Grounded;
			
			// 착지 순간(SnapToGround 조건): 평지 착지처럼 확실한 바닥일 때만 스냅해서 튐/관통/뜬 상태를 줄임
			// 경사에서는 무리하게 스냅하면 흔들림이 생겨서 제한
			const bool bJustLanded = PrevMode == ESkullyMovementMode::Falling;
			if (bJustLanded == true && bWalkable == true && bIsSlope == false && Velocity.Z <= 0.0f)
			{
				SnapToGround(Hit);
			}
			
			return;
		}
	}
	
	// Sweep이 실패하거나 애매하면, 짧은 아래 라인트레이스로 바닥 재확인
	// 특히 엣지/꼭짓점에서 Sweep 결과가 불안정할 때 붙잡아주는 역할
	{
		FHitResult LineHit;
		const FVector StartPos = UpdatedComponent->GetComponentLocation();
		const FVector EndPos = StartPos - FVector::UpVector * GroundLineTraceDistance;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
		Params.AddIgnoredActor(GetOwner());
		// FaceIndex를 받게 설정
		Params.bReturnFaceIndex = true;
		// 삼각형 기반(Complex)으로 받게 설정
		Params.bTraceComplex = true;
		
		if (GetWorld()->LineTraceSingleByChannel(LineHit, StartPos, EndPos, ECC_Visibility, Params))
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

void USkullyMovementComponent::UpdateMotionState()
{
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
	CurrentSpeed2D = Velocity2D.Size();
	
	CurrentMoveDir2D = (CurrentSpeed2D > KINDA_SMALL_NUMBER) ? Velocity2D / CurrentSpeed2D : FVector::ZeroVector;
}

// 바닥 감지: 현재 구의 위치에서 아래로 Sphere Sweep하여 정보를 반환
bool USkullyMovementComponent::SweepGround(FHitResult& OutHit)
{
	// 스컬리는 구형이므로 SphereComponent를 기준으로 함
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return false;
	}

	const float Radius = Sphere->GetScaledSphereRadius();
	// 스윕 시작 지점은 구의 중심
	const FVector StartPos = UpdatedComponent->GetComponentLocation();
	// 스윕 감지 지점은 구의 반지름 + GroundCheckDistance만큼의 아래 방향 지점
	const FVector EndPos = StartPos - FVector::UpVector * (Radius + GroundCheckDistance);

	// 자기 자신은 무시
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
	Params.AddIgnoredActor(GetOwner());
	// FaceIndex를 받게 설정
	Params.bReturnFaceIndex = true;
	// FaceIndex를 삼각형 기반(Complex)으로 받게 설정
	Params.bTraceComplex = true;

	// StartPos 지점에서 EndPos 지점까지 Radius 반지름만큼의 구를 Sweep하여 출력 매개 변수인 OutHit에 정보를 반환 
	return GetWorld()->SweepSingleByChannel(OutHit, StartPos, EndPos, FQuat::Identity, ECC_Visibility,
	                                        FCollisionShape::MakeSphere(Radius), Params);
}

// 구체 캐릭터가 바닥에 정확히 닿도록 위치를 보정
// 착지 순간 지면에 파고들거나 떠 있는 오차를 제거하기 위한 스냅
void USkullyMovementComponent::SnapToGround(const FHitResult& Hit)
{
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return;
	}

	// ImpactPoint에 구 반지름만큼 노멀 방향으로 올려서 구가 바닥에 딱 얹히게 함
	const float Radius = Sphere->GetScaledSphereRadius();
	const FVector TargetLocation = Hit.ImpactPoint + Hit.ImpactNormal * Radius;

	UpdatedComponent->SetWorldLocation(TargetLocation);
}

namespace
{
	// 엣지/경계에서 CachedFloorNormal이 튀면서 슬라이드/투영이 0으로 붕괴할 수 있어, 
	// 주변 바닥 높이를 샘플링해서 가장 아래로 향하는 downhill 방향을 구한다.
	bool TryGetDownhillDirFromSamples(UWorld* World, const FVector& Origin, float SampleDist, float TraceDown, AActor* IgnoreActor, FVector& OutDir)
	{
		if (World == nullptr || SampleDist <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyDownhillSample), false);
		if (IgnoreActor != nullptr)
		{
			Params.AddIgnoredActor(IgnoreActor);;
		}
		Params.bTraceComplex = true;
		Params.bReturnFaceIndex = true;
		
		// 기준 높이
		float BaseZ = Origin.Z;
		float BestZ = BaseZ;
		FVector BestDir = FVector::ZeroVector;
		
		const FVector Dirs[4] = { FVector::ForwardVector, FVector::RightVector, -FVector::ForwardVector, -FVector::RightVector};
		for (const FVector& Dir : Dirs)
		{
			const FVector SampleStart = Origin + Dir * SampleDist;
			const FVector SampleEnd = SampleStart - FVector::UpVector * TraceDown;
			
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, SampleStart, SampleEnd, ECC_Visibility, Params) && Hit.bBlockingHit == true)
			{
				const float Z = Hit.ImpactPoint.Z;
				if (Z < BestZ)
				{
					BestZ = Z;
					BestDir = Dir;
				}
			}
		}
		
		// 충분히 아래가 아니면 무시
		if (BestDir.IsNearlyZero() == true)
		{
			return false;
		}
		
		OutDir = BestDir.GetSafeNormal();
		
		return true;
	}
}
