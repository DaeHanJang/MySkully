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

	if (JumpBufferRemaining > 0.0f)
	{
		JumpBufferRemaining -= DeltaTime;
		if (JumpBufferRemaining <= 0.0f)
		{
			bWantsToJump = false;
			JumpBufferRemaining = 0.0f;
		}
	}
	PendingSlopeSlideAccel2D = FVector::ZeroVector;
	bSlopeSlideThisFrame = false;
	
	// 점프 적용
	TryConsumeJump();
	// 중력 적용(Falling일 때만 Z 하강(아래로 가속))
	ApplyGravity(DeltaTime);
	// 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
	bSlopeSlideAppliedThisFrame = ApplySlopeSlide(DeltaTime);
	const bool bFrictionAsSlide = bIsSlopeSliding == true || bSlopeSlideThisFrame == true || bSlopeSlideAppliedThisFrame == true;
	// 마찰 적용(XY 감속(XY 속도를 줄여 미끄러짐/관성을 제어))
	ApplyFriction(DeltaTime, bFrictionAsSlide ? SlidingFriction : GroundFriction);
	// 이동 처리(Sweep 기반 이동 + 충돌 처리(입력 기반 + 경사 투영 + 불안정 바닥 처리))
	Move(DeltaTime);
	// 지면 재판정(Sweep + LineTrace로 Grounded/Falling 갱신)
	// Move()가 먼저 움직인 뒤, CheckGround()가 새 위치에서 바닥 상태를 확정
	CheckGround(DeltaTime);
	// 매쉬 회전
	ApplyVisualRoll(LastActualDelta);
	TryConsumeJump();
	// 이동 상태값(현재 속력, 방향 등) 갱신
	UpdateMotionState();
}

// 중력 적용
void USkullyMovementComponent::ApplyGravity(float DeltaTime)
{
	// Falling일 때만 중력 가속을 적용해서 낙하
	if (MovementMode == ESkullyMovementMode::Falling)
	{
		const float Scale = (Velocity.Z > 0.0f) ? JumpGravityScale : FallGravityScale;
		Velocity.Z -= Gravity * Scale * DeltaTime;
	}
	// Grounded 상태에서는 중력으로 바닥을 파고들지 않도록 Z 속도를 최소 0으로 유지
	else
	{
		Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
	}
}

void USkullyMovementComponent::TryConsumeJump()
{
	if (bWantsToJump == false)
	{
		return;
	}
	
	if (CanJump() == true)
	{
		Velocity.Z = JumpSpeed;
		MovementMode = ESkullyMovementMode::Falling;
		bIsSlopeSliding = false;
		bWantsToJump = false;
		JumpBufferRemaining = 0.0f;
		JumpIgnoreGroundRemaining = JumpIgnoreGroundTime;
	}
}

bool USkullyMovementComponent::CanJump() const
{
	return MovementMode == ESkullyMovementMode::Grounded;
}

// 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
bool USkullyMovementComponent::ApplySlopeSlide(float DeltaTime)
{
	// 기본 전제: Grounded + 입력 없음일 때만 정지 후 굴러떨어짐을 평가한다.
	if (MovementMode != ESkullyMovementMode::Grounded)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 불안정 바닥에서는 CachedFloorNormal이 튀는 프레임이 있으므로, 슬라이드에 사용할 노멀을 안정화
	const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
	const bool bUnstableForSlide = (CachedFloorNormal.Z < UnstableFloorZThreshold) || (NormalDot < FloorNormalDotEdgeThreshold);
	FVector UseNormal = (bUnstableForSlide ? LastFloorNormal : CachedFloorNormal).GetSafeNormal();
	if (UseNormal.IsNearlyZero())
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 중력의 바닥 평면 성분(경사 아래 가속)
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UseNormal);
	
	// 엣지/경계 보강(노멀이 애매해서 방향(AlongPlane)이 거의 0이면 주변 샘플로 downhill 방향을 추정)
	const float MinSlopeForSamplesZ = 0.98f;
	if (AlongPlane.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		FVector DownhillDir;
		if (UseNormal.Z < MinSlopeForSamplesZ && 
			TryGetDownhillDirFromSamples(GetWorld(), UpdatedComponent->GetComponentLocation(), DownhillSampleDistance, GroundLineTraceDistance + 50.0f, GetOwner(), DownhillDir))
		{
			// 방향만 필요하므로 크기는 적당히
			AlongPlane = DownhillDir * (Gravity * (1.0f - UseNormal.Z));
		}
		else
		{
			bIsSlopeSliding = false;
			return false;
		}
	}
	
	// 슬라이드 방향
	FVector SlideDir = AlongPlane.GetSafeNormal();
	if (SlideDir.IsNearlyZero() == true)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 경사량: 0(평지)~1(수직)
	const float SlopeAmount = FMath::Clamp(1.0f - UseNormal.Z, 0.0f, 1.0f);
	// 커브: 지수 < 1이면 완만한 경사도 좀 더 잘 미끄러짐
	const float Curve = FMath::Pow(SlopeAmount, 0.7f);
	// 가속 크기(게임 감성)
	float SlideAccelMag = Gravity * Curve * SlopeSlideScale;
	// 최소 가속 보장(너무 느린 체감 방지)
	const float MinSlideAccel = 800.0f;
	SlideAccelMag = FMath::Max(SlideAccelMag, MinSlideAccel);
	// 너무 작으면 슬라이드 불가(안전)
	if (SlideAccelMag <= KINDA_SMALL_NUMBER)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 슬라이딩 종료(히스테리시스): 슬라이딩 중일 때 너무 약해지면 종료
	const float StopThreshold = StaticFrictionAccel * 0.25f;
	if (bIsSlopeSliding == true && SlideAccelMag < StopThreshold)
	{
		bIsSlopeSliding = false;
		return false;
	}
	
	// 시작(Static friction): 아직 슬라이딩 중이 아니면 정지 마찰을 이겨야 시작
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
	}
	
	// 슬라이드 가속을 Move()에서 더할 수 있도록 저장
	PendingSlopeSlideAccel2D = FVector(SlideDir.X, SlideDir.Y, 0.0f) * SlideAccelMag;
	bSlopeSlideThisFrame = true;
	return true;
}

// 마찰 적용
void USkullyMovementComponent::ApplyFriction(float DeltaTime, float GroundedFriction)
{
	const bool bGrounded = MovementMode == ESkullyMovementMode::Grounded;
	// 슬라이드 상태 판단(이번 프레임 슬라이드 가속이 있거나, 슬라이딩 플래그가 켜진 경우)
	const bool bSlidingNow = bGrounded == true && (bIsSlopeSliding == true || bSlopeSlideThisFrame == true || bSlopeSlideAppliedThisFrame == true);
	
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	if (HorizontalVelocity.IsNearlyZero() == true)
	{
		return;
	}
	
	if (bSlidingNow == true)
	{
		// 슬라이딩 중에는 속도 곱 감쇠를 쓰지 말자(너무 세게 죽음)
		// 대신 아주 약한 상수 마찰만 적용하거나, 아예 0으로 둬도 됨
		// SlidingFriction 값을 여기서 쓰되 상수 감속으로만 쓴다
		const float Friction = SlidingFriction;
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
	else
	{
		// 일반 지면/공중 마찰
		const float Friction = bGrounded ? GroundedFriction : AirFriction;
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
	
	Velocity.X = HorizontalVelocity.X;
	Velocity.Y = HorizontalVelocity.Y;
}

// 이동 처리
void USkullyMovementComponent::Move(float DeltaTime)
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	
	// 입력 소비
	const FVector Input = ConsumeMovementInput();
	const bool bHasInput = Input.IsNearlyZero() == false;
	const FVector InputDir = bHasInput ? Input.GetSafeNormal() : FVector::ZeroVector;
	
	// 입력 목표 속도(arcade) 설정
	FVector TargetVelocity2D = FVector::ZeroVector;
	if (bHasInput == true)
	{
		// 슬라이드 프리임이면 목표 속도 상한을 더 크게 잡을 수도 있지만,
		// 보통은 입력은 MaxSpeed, 슬라이드는 별도로 더해지는 구조가 안정적임.
		TargetVelocity2D = FVector(InputDir.X, InputDir.Y, 0.0f) * MaxSpeed;
	}
	
	// 현재 수평 속도
	FVector CurrentVelocity2D(Velocity.X, Velocity.Y, 0.0f);
	
	// 입력 가속(입력이 있을 때만)
	if (TargetVelocity2D.IsNearlyZero() == false)
	{
		CurrentVelocity2D = FMath::VInterpConstantTo(CurrentVelocity2D, TargetVelocity2D, DeltaTime, Acceleration);
	}
	
	// 슬라이드 가속 누적(입력과 독립)
	if (MovementMode == ESkullyMovementMode::Grounded && bSlopeSlideThisFrame == true)
	{
		CurrentVelocity2D += PendingSlopeSlideAccel2D * DeltaTime;
	}
	
	// 속도 클램프(슬라이드 중이면 더 큰 상한)
	const bool bClampAsSlope = MovementMode == ESkullyMovementMode::Grounded && bSlopeSlideThisFrame == true;
	CurrentVelocity2D = CurrentVelocity2D.GetClampedToMaxSize(bClampAsSlope ? MaxSlopeSlideSpeed : MaxSpeed);
	
	Velocity.X = CurrentVelocity2D.X;
	Velocity.Y = CurrentVelocity2D.Y;
	
	// 이번 프레임 이동량
	FVector MoveDelta = Velocity * DeltaTime;

	// Grounded면 바닥 평면으로 투영 + 엣지 보정
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
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

		FVector AdjustedMove = FVector::ZeroVector;
		// 슬라이드로 이미 자연스러운 속도가 만들어진 프레임에는
		// 투영 붕괴(엣지/경계)에서의 최소 이동 보장(MinProjectedMoveCm)이 에너지를 인위적으로 주입할 수 있다.
		// 입력이 있는 경우엔 조작감/정지 방지가 중요하니 최소 이동 보장을 허용
		// 입력이 없고(정지 후 굴러떨어짐) 슬라이드가 적용된 프레임에는 최소 이동 보장을 끈다
		const bool bAllowMinMoveGuarantee = bHasInput;
		
		// 불안정 바닥이면
		if (bUnstableFloor == true)
		{
			const FVector FallbackDir = bHasInput ? InputDir : VelocityDir2D;
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
				const FVector FallbackDir = bHasInput ? InputDir : VelocityDir2D;
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
	
	// 실제 이동(Sweep)
	FHitResult Hit;
	// Sweep 이동(관통 방지) + Hit 결과를 돌려줌
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

	// 막혔으면 벽 슬라이드 + 벽 감쇠
	if (Hit.bBlockingHit == true)
	{
		// 벽을 타고 미끄러짐 시도
		SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);

		FVector Vel2D(Velocity.X, Velocity.Y, 0.0f);
		const float Speed2D = Vel2D.Size();
		if (Speed2D > KINDA_SMALL_NUMBER)
		{
			// 수평에서의 벽 노멀
			FVector WallN = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
			if (!WallN.IsNearlyZero())
			{
				// 노멀 성분(벽을 "미는" 성분) 제거/감쇠
				const float IntoWall = FVector::DotProduct(Vel2D, WallN);
				if (IntoWall > 0.f)
				{
					Vel2D -= WallN * (IntoWall * WallNormalKill);
				}

				// 접선 성분 감쇠(벽 타고 가는 힘 줄이기)
				const float Factor = FMath::Clamp(1.0f - WallSlideDamping * DeltaTime, 0.0f, 1.0f);
				Vel2D *= Factor;

				Velocity.X = Vel2D.X;
				Velocity.Y = Vel2D.Y;
			}
		}
		
		// Grounded 2-pass(슬라이드 프레임이면 에너지 폭발 방지로 스킵)
		// Grounded면 한 번 더 바닥 기준 재투영 이동을 짧게 시도
		// 경계(Edge)에서 막힐 때 Hit.Normal은 벽도 아니고 바닥도 아닌 애매한 노멀일 수 있으므로 1회 슬라이드로는 이동이 소실되기 쉬움
		// Edge/Corner에서는 1회 슬라이드로 이동이 0이 될 수 있어서 2-pass로 재시도한다
		// 1차: 벽 노멀 기준 슬라이드
		// 2차: 바닥 노멀 기준 재투영 짧은 이동 시도
		if (MovementMode == ESkullyMovementMode::Grounded)
		{
			// 경사 슬라이드가 적용된 프레임엔 2-pass(바닥 재투영)로 이동을 한 번 더 시도하면
			// 엣지/경계에서 한 프레임에 이동이 두 번 발생하며 거리 폭발이 생길 수 있다.
			if (bSlopeSlideAppliedThisFrame == false)
			{
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
	
	// 실제 이동량 기록(비주얼 롤링용)
	const FVector End = UpdatedComponent->GetComponentLocation();
	LastActualDelta = End - Start;
}

void USkullyMovementComponent::ApplyVisualRoll(const FVector& ActualDelta)
{
	if (bRollVisualOnMove == false)
	{
		return;
	}

	USceneComponent* Target = VisualComponent ? VisualComponent : GetOwner()->GetRootComponent();
	if (Target == nullptr)
	{
		return;
	}

	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return;
	}
	
	const float Radius = Sphere->GetScaledSphereRadius();
	if (Radius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FVector Up = (MovementMode == ESkullyMovementMode::Grounded) ? CachedFloorNormal.GetSafeNormal() : FVector::UpVector;
	if (Up.IsNearlyZero() == true)
	{
		Up = FVector::UpVector;
		Up.Normalize();
	}
	// 경사면 포함: 실제 이동을 바닥 평면으로 투영한 굴림용 이동
	const FVector RollDelta = FVector::VectorPlaneProject(ActualDelta, Up);
	const float Dist = RollDelta.Size();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector Dir = RollDelta / Dist;
	
	const FVector Axis = FVector::CrossProduct(Up, Dir).GetSafeNormal();
	if (Axis.IsNearlyZero() == true)
	{
		return;
	}

	const float Angle = (Dist / Radius) * RollVisualScale;
	const FQuat DeltaRot(Axis, Angle);

	Target->AddWorldRotation(DeltaRot);
}

// 지면 판정
// Sweep -> 실패/애매하면 LineTrace 보조
// WalkableZ/GraceZ로 Grounded 깜빡임 완화
// CachedFloorNormal은 이동 투영에 쓰이므로 보간으로 안정화
void USkullyMovementComponent::CheckGround(float DeltaTime)
{
	// 점프 직후 일정 시간 바닥 판정 무시
	if (JumpIgnoreGroundRemaining > 0.0f)
	{
		JumpIgnoreGroundRemaining -= DeltaTime;
		MovementMode = ESkullyMovementMode::Falling;
		bOnUnwalkableSlope = false;
		return;
	}
	
	FHitResult Hit;
	const bool bHitGround = SweepGround(Hit);
	
	// 이 경사면이 걸을 수 있는 바닥인가를 판정(허용 경사각의 기준선) 
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle));
	// Walkable 판정이 살짝 흔들려도(노멀 튐) Grounded 상태를 유지하는 관용값
	// 없으면 경계면/경계/꼭짓점에서 노멀 값이 할 프레임씩 튀면서 Grounded <-> Falling이 깜빡거림
	// 그 결과 이동이 끊기고 미세한 떨림, 툭툭 끊기는 느낌이 남
	const float GraceZ = WalkableZ - GroundGraceZOffset;
	
	bOnUnwalkableSlope = false;

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
		
		// Unwalkable이지만 붙어있는 경사면->슬라이딩 상태로 취급
		// 거의 벽(절벽)까지 붙어서 내려가는 걸 막고 싶으면 하한을 둠
		const float MinSlopeZForSlide = 0.1f;
		if (HitZ > MinSlopeZForSlide)
		{
			bOnUnwalkableSlope = true;
			UnwalkableNormal = Hit.ImpactNormal.GetSafeNormal();
		
			CurrentFloorHit = Hit;
			LastFloorNormal = CachedFloorNormal;
			CachedFloorNormal = UnwalkableNormal;
			
			// 바닥에 붙어있는 동안은 Grounded로 유지
			MovementMode = ESkullyMovementMode::Grounded;
			return;
		}

		// Grace 판정(붙어있기 관용)
		// 아주 약간 기준보다 나빠져도 즉시 Falling으로 바꾸지 않고 유지
		// 경계/폴리곤 경계에서 Grounded가 깜빡이는 현상(Ground jitter) 방지
		// 여기까지 왔다는 건: 너무 가파라서 슬라이드로도 취급 안 하는 수준(거의 벽)
		// 이전 프레임이 Grounded였고, 노멀만 살짝 튄 경우를 붙잡는 용도
		if (MovementMode == ESkullyMovementMode::Grounded && HitZ >= GraceZ)
		{
			CurrentFloorHit = Hit;
			LastFloorNormal = CachedFloorNormal;
			CachedFloorNormal = Hit.ImpactNormal.GetSafeNormal();
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
				bOnUnwalkableSlope = false;
				return;
			}
		}
	}

	CurrentFloorHit = FHitResult();
	MovementMode = ESkullyMovementMode::Falling;
	bOnUnwalkableSlope = false;
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

void USkullyMovementComponent::UpdateMotionState()
{
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
	CurrentSpeed2D = Velocity2D.Size();
	
	CurrentMoveDir2D = (CurrentSpeed2D > KINDA_SMALL_NUMBER) ? Velocity2D / CurrentSpeed2D : FVector::ZeroVector;
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

void USkullyMovementComponent::RequestJump()
{
	// 이미 누르고 있으면(홀드) 추가 요청은 무시
	if (bJumpHeld == true)
	{
		return;
	}
	
	bJumpHeld = true;
	bWantsToJump = true;
	JumpBufferRemaining = JumpBufferTime;
}

void USkullyMovementComponent::RequestJumpRelease()
{
	bJumpHeld = false;
	
	if (MovementMode == ESkullyMovementMode::Falling && Velocity.Z > 0.0f)
	{
		Velocity.Z *= 0.7f;
	}
}
