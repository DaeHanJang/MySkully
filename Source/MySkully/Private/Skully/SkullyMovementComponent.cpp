// Fill out your copyright notice in the Description page of Project Settings.


#include "Skully/SkullyMovementComponent.h"

#include "Components/SphereComponent.h"

namespace
{
	// 엣지/경계에서 CachedFloorNormal이 튀면서 슬라이드/투영이 0으로 붕괴할 수 있어, 
	// 주변 바닥 높이를 샘플링해서 가장 아래로 향하는 downhill 방향을 구한다.
	static bool TryGetDownhillDirFromSamples(UWorld* World, const FVector& Origin, float SampleDist, float TraceDown, AActor* IgnoreActor, FVector& OutDir)
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
	// 지면 재판정(Sweep + LineTrace로 Grounded/Falling 갱신)
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
		Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
	}
}

// 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
bool USkullyMovementComponent::ApplySlopeSlide(float DeltaTime)
{
	// 기본 전제: Grounded + 입력 없음일 때만 정지 후 굴러떨어짐을 평가한다.
	if (MovementMode != ESkullyMovementMode::Grounded || GetPendingInputVector().IsNearlyZero() == false)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// const FVector Vel2D(Velocity.X, Velocity.Y, 0.0f);
	// const bool bNearlyStopped = (Vel2D.SizeSquared() < FMath::Square(10.0f));
	//
	// if (bIsSlopeSliding == false && bNearlyStopped == false)
	// {
	// 	// 아직 슬라이딩 시작 전인데 이미 움직이는 중이면 시작 안 함
	// 	return false;
	// }
	
	// 불안정 바닥에서는 CachedFloorNormal이 튀는 프레임이 있으므로, 슬라이드에 사용할 노멀을 안정화
	const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
	const bool bUnstableForSlide = (CachedFloorNormal.Z < UnstableFloorZThreshold) || (NormalDot < FloorNormalDotEdgeThreshold);
	FVector UseNormal = (bUnstableForSlide ? LastFloorNormal : CachedFloorNormal);
	UseNormal = UseNormal.GetSafeNormal();
	
	// 중력의 바닥 평면 성분(경사 아래 가속)
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UseNormal);
	
	const float MinSlopeForSamplesZ = 0.98f;
	// 엣지/경계에서 노멀이 애매해서 AlongPlane이 거의 0이 되면, 주변 샘플로 downhill 방향을 보강
	if (AlongPlane.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		FVector DownhillDir;
		if (UseNormal.Z < MinSlopeForSamplesZ && 
			TryGetDownhillDirFromSamples(GetWorld(), UpdatedComponent->GetComponentLocation(), DownhillSampleDistance, GroundLineTraceDistance + 50.0f, GetOwner(), DownhillDir))
		{
			AlongPlane = DownhillDir * Gravity; // 방향만 필요하므로 크기는 적당히(Scale은 아래에서 적용)
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
	
	// 방향
	if (SlideAccelMag <= KINDA_SMALL_NUMBER)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	const FVector SlideDir = SlideAccel / SlideAccelMag;
	
	// 슬라이딩 종료(히스테리시스): 슬라이딩 중일 때 너무 약해지면 종료
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
	// 현재 프레임 입력 방향을 받아서 Velocity.X/Y를 설정
	// 이 구조는 가속(accel) 모델이 아니라, 입력이 있으면 목표 속도를 즉시 세팅하는 방식(arcade 타입)
	const FVector Input = ConsumeMovementInput();

	FVector TargetVelocity2D = FVector::ZeroVector;
	
	if (Input.IsNearlyZero() == false)
	{
		const FVector InputDir = Input.GetSafeNormal();

		TargetVelocity2D = FVector(InputDir.X, InputDir.Y, 0.0f) * MaxSpeed;
	}
	
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

	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		const bool bHasInput = !Input.IsNearlyZero();
		const FVector InputDir = bHasInput ? Input.GetSafeNormal() : FVector::ZeroVector;
		const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		const float Speed2D = Velocity2D.Size();
		const FVector VelocityDir2D = (Speed2D > KINDA_SMALL_NUMBER) ? (Velocity2D / Speed2D) : FVector::ZeroVector;

		// Grounded일 때 이동 보정(경계/경사)
		// 바닥 노멀을 믿기 어려운 상황(경계/꼭짓점/급격한 노멀 변동)을 감지하는 플래그
		// 경계/꼭지점에서 PlaneProject가 이동 벡터를 0으로 만들 수 있으므로, 이때는 투영 대신 입력 기반 이동으로 탈출
		const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
		// FaceIndex는 단순 콜리전/프리미티브 바닥에서는 INDEX_NONE이 정상일 수 있어, 
		// 기본적으로 불안정 판정에 넣지 않는다(맵/콜리전에 따라 옵션화 가능)
		const bool bUnstableFloor = 
			CachedFloorNormal.Z < UnstableFloorZThreshold || // 바닥이 충분히 평평하지 않거나(경사/급경사), 노멀 자체가 수직에 가까운 특이 케이스 가능성
			NormalDot < FloorNormalDotEdgeThreshold; // 이전 프레임 노멀과 현재 노멀이 급격히 달라짐(면-면 경계/엣지 가능성)

		FVector AdjustedMove;
		// 슬라이드로 이미 자연스러운 속도가 만들어진 프레임에는
		// 투영 붕괴(엣지/경계)에서의 최소 이동 보장(MinProjectedMoveCm)이 에너지를 인위적으로 주입할 수 있다.
		// 입력이 있는 경우엔 조작감/정지 방지가 중요하니 최소 이동 보장을 허용
		// 입력이 없고(정지 후 굴러떨어짐) 슬라이드가 적용된 프레임에는 최소 이동 보장을 끈다
		const bool bAllowMinMoveGuarantee = bHasInput;
		
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
	
	// 실제 이동(Sweep) + 충돌 처리
	FHitResult Hit;
	// Sweep 이동(관통 방지) + Hit 결과를 돌려줌
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

	// 막혔으면
	if (Hit.bBlockingHit == true)
	{
		// 벽을 타고 미끄러짐 시도
		SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);

		// Grounded면 한 번 더 바닥 기준 재투영 이동을 짧게 시도
		// 경계(Edge)에서 막힐 때 Hit.Normal은 벽도 아니고 바닥도 아닌 애매한 노멀일 수 있으므로 1회 슬라이드로는 이동이 소실되기 쉬움
		// Edge/Corner에서는 1회 슬라이드로 이동이 0이 될 수 있어서 2-pass로 재시도한다
		// 1차: 벽 노멀 기준 슬라이드
		// 2차: 바닥 노멀 기준 재투영 짧은 이동 시도
		if (MovementMode == ESkullyMovementMode::Grounded)
		{
			// 경사 슬라이드가 적용된 프레임엔 2-pass(바닥 재투영)로 이동을 한 번 더 시도하면
			// 엣지/경계에서 한 프레임에 이동이 두 번 발생하며 거리 폭발이 생길 수 있다.
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
}

// 지면 판정
// Sweep -> 실패/애매하면 LineTrace 보조
// WalkableZ/GraceZ로 Grounded 깜빡임 완화
// CachedFloorNormal은 이동 투영에 쓰이므로 보간으로 안정화
void USkullyMovementComponent::CheckGround(float DeltaTime)
{
	FHitResult Hit;
	const bool bHitGround = SweepGround(Hit);
	
	// 이 경사면이 걸을 수 있는 바닥인가를 판정(허용 경사각의 기준선) 
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	// Walkable 판정이 살짝 흔들려도(노멀 튐) Grounded 상태를 유지하는 관용값
	// 없으면 경계면/경계/꼭짓점에서 노멀 값이 할 프레임씩 튀면서 Grounded <-> Falling이 깜빡거림
	// 그 결과 이동이 끊기고 미세한 떨림, 툭툭 끊기는 느낌이 남
	const float GraceZ = WalkableZ - GroundGraceZOffset;

	// 바닥이 충분히 가까이 있고(거리), 블로킹 히트면 유효 바닥 후보
	if (bHitGround == true && Hit.bBlockingHit == true && Hit.Distance <= MaxGroundDistance)
	{
		// 바닥 노멀의 Z(1: 완전한 평지, 작아질수록: 가파른 평지)
		const float HitZ = Hit.ImpactNormal.Z;

		// 걸을 수 있는 경사로 판단
		if (HitZ >= WalkableZ)
		{
			CurrentFloorHit = Hit;
			
			const bool bIsSlope = HitZ < FlatGroundZThreshold;
			
			// 평지에 가까우면: 보간하지 말고 즉시 업벡터로 스냅 (경사 잔상 제거)
			if (bIsSlope == false)
			{
				LastFloorNormal = FVector::UpVector;
				CachedFloorNormal = FVector::UpVector;
				
				// 평지에 닿았으면 경사 슬라이딩 상태도 끊어주는 게 안전
				bIsSlopeSliding = false;
			}			
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
			
			// 착지 순간 SnapToGround 조건
			// 평지 착지처럼 확실한 바닥일 때만 스냅해서 튐/관통/뜬 상태를 줄임
			// 경사에서는 무리하게 스냅하면 흔들림이 생겨서 제한
			if (bIsSlope == false && MovementMode == ESkullyMovementMode::Falling && Velocity.Z <= 0.0f)
			{
				SnapToGround(Hit);
			}

			MovementMode = ESkullyMovementMode::Grounded;

			return;
		}

		// Grace 판정(붙어있기 관용)
		// 아주 약간 기준보다 나빠져도 즉시 Falling으로 바꾸지 않고 유지
		// 경계/폴리곤 경계에서 Grounded가 깜빡이는 현상(Ground jitter) 방지
		if (MovementMode == ESkullyMovementMode::Grounded && HitZ >= GraceZ)
		{
			CurrentFloorHit = Hit;
			CachedFloorNormal = Hit.ImpactNormal;
			return;
		}
	}
	// Sweep가 실패하거나 애매하면, 짧은 아래 라인트레이스로 바닥 재확인
	// 특히 엣지/꼭짓점에서 Sweep 결과가 불안정할 때 붙잡아주는 역할
	{
		FHitResult LineHit;
		const FVector Start = UpdatedComponent->GetComponentLocation();
		const FVector End = Start - FVector::UpVector * GroundLineTraceDistance;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
		Params.AddIgnoredActor(GetOwner());

		// FaceIndex를 받게 설정
		Params.bReturnFaceIndex = true;
		// 삼각형 기반(Complex)으로 받게 설정
		Params.bTraceComplex = true;
		
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

void USkullyMovementComponent::UpdateMotionState()
{
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
	CurrentSpeed2D = Velocity2D.Size();
	
	CurrentMoveDir2D = (CurrentSpeed2D > KINDA_SMALL_NUMBER) ? Velocity2D / CurrentSpeed2D : FVector::ZeroVector;
}

// 바닥 감지
// 현재 위치에서 아래로 Sphere Sweep 해서 바닥을 찾음
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
	// 구의 바닥면이 닿을 수 있는 범위까지 아래로 스윕
	const FVector End = Start - FVector::UpVector * (Radius + GroundCheckDistance);

	// 자기 자신은 무시
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
	Params.AddIgnoredActor(GetOwner());
	
	// FaceIndex를 받게 설정
	Params.bReturnFaceIndex = true;
	// 삼각형 기반(Complex)으로 받게 설정
	Params.bTraceComplex = true;

	// Radius 반지름의 구가 Start 지점에서 End 지점까지 부딪치는 것이 인지는 확인(지면과의 접촉/거리/노멀을 얻음)
	return GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Visibility,
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
