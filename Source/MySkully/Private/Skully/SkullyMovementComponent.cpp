// Fill out your copyright notice in the Description page of Project Settings.


#include "Skully/SkullyMovementComponent.h"

#include "Components/SphereComponent.h"

namespace
{
	// 엣지/경계에서 CachedFloorNormal이 튀면서 슬라이드/투영이 0으로 붕괴할 수 있어, 
	// 주변 바닥 높이를 샘플링해서 가장 아래로 향하는 downhill 방향을 구한다.
	bool TryGetDownhillDirFromSamples(UWorld* World, const FVector& Origin, float SampleDist, float TraceDown,
	                                  AActor* IgnoreActor, FVector& OutDir)
	{
		if (World == nullptr || SampleDist <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyDownhillSample), false);
		if (IgnoreActor != nullptr)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		Params.bTraceComplex = true;
		Params.bReturnFaceIndex = true;

		// 기준 높이
		float BaseZ = Origin.Z;
		float BestZ = BaseZ;
		FVector BestDir = FVector::ZeroVector;

		const FVector Dirs[4] = {
			FVector::ForwardVector, FVector::RightVector, -FVector::ForwardVector, -FVector::RightVector
		};
		for (const FVector& Dir : Dirs)
		{
			const FVector SampleStart = Origin + Dir * SampleDist;
			const FVector SampleEnd = SampleStart - FVector::UpVector * TraceDown;

			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, SampleStart, SampleEnd, ECC_Visibility, Params) == true && Hit.bBlockingHit == true)
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
	// 이 컴포넌트는 매 Tick마다 자체 물리(중력/마찰/이동/지면판정)를 처리
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

	PendingSlopeSlideAccel2D = FVector::ZeroVector;
	bSlopeSlideThisFrame = false;

	/********************점프********************/
	UpdateJumpBufferTimer(DeltaTime);
	TryStartJumpFromBuffer();
	TryConsumeJump();
	/********************중력********************/
	ApplyGravity(DeltaTime);
	/******************미끄러짐*******************/
	ApplyUnwalkableSlide(DeltaTime); // 걸을 수 없는 바닥 슬라이드 가속
	bSlopeSlideAppliedThisFrame = ApplySlopeSlide(DeltaTime); // 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
	const bool bFrictionAsSlide = bIsSlopeSliding == true || bSlopeSlideThisFrame == true || bSlopeSlideAppliedThisFrame == true;
	const bool bNoInput = GetPendingInputVector().IsNearlyZero() == true;
	const bool bSlope = CachedFloorNormal.Z < FlatGroundZThreshold;
	const bool bTreatAsSlideFriction = bFrictionAsSlide == true || (MovementMode == ESkullyMovementMode::Grounded && bNoInput == true && bSlope == true);
	ApplyFriction(DeltaTime, bTreatAsSlideFriction ? SlidingFriction : GroundFriction); // 마찰 적용(XY 감속(XY 속도를 줄여 미끄러짐/관성을 제어))
	/********************이동********************/
	Move(DeltaTime);
	/******************바닥 감지******************/
	CheckGround(DeltaTime);
	/*****************구르기 연출*****************/
	ApplyVisualRoll(LastActualDelta);
	/******************출력 변수******************/
	UpdateMotionState();
}

void USkullyMovementComponent::ApplyGravity(float DeltaTime)
{
	if (MovementMode == ESkullyMovementMode::Falling)
	{
		// 점프는 약하게 낙하는 빠르게: 조작감(공중 컨트롤, 반응성)+템포 상승
		const float Scale = (Velocity.Z > 0.0f) ? JumpGravityScale : FallGravityScale;
		Velocity.Z -= Gravity * Scale * DeltaTime;
	}
	else
	{
		if (bOnUnwalkableSlope == false)
		{
			const bool bSlope = CachedFloorNormal.Z < FlatGroundZThreshold; // 경사면 일 경우
			const bool bNoInput = GetPendingInputVector().IsNearlyZero() == true; // 입력이 없을 경우
			// 슬라이딩 중인 경우
			const bool bSlidingLike = bIsSlopeSliding == true || 
				                      bSlopeSlideThisFrame == true || 
				                      bSlopeSlideAppliedThisFrame == true;

			// 평지
			if (bSlope == false)
			{
				Velocity.Z = 0.0f;
			}
			// 경사: 접지(안정화)->떨림, 경사면에서 붕 뜨는 현상 완화
			else
			{
				Velocity.Z = FMath::Clamp(Velocity.Z, -StickZ, 0.0f);

				// 입력이 없고 미끄러지는 상태일 경우
				if (bNoInput == true && bSlidingLike == true)
				{
					// 접지를 더 강하게->프레임마다 접지가 끊겼다가 다시 잡힐 수 있음(슬라이딩 판정 깜빡임, 떨림)
					Velocity.Z = FMath::Min(Velocity.Z, -StickZ);
				}
			}
		}
	}
}

void USkullyMovementComponent::ApplyUnwalkableSlide(float DeltaTime)
{
	if (bOnUnwalkableSlope == false)
	{
		return;
	}

	// 너무 가파른 면의 노멀
	const FVector N = UnwalkableNormal.GetSafeNormal();
	if (N.IsNearlyZero() == true)
	{
		return;
	}

	// 중력을 면에 투영 -> 면을 따라 아래로 가는 방향
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, N);

	// 2D로만 굴리고 싶으면 Z 제거
	AlongPlane.Z = 0.0f;

	const FVector SlideDir = AlongPlane.GetSafeNormal();
	if (SlideDir.IsNearlyZero() == true)
	{
		return;
	}

	// 자석을 끊기 위해 최소 가속을 강하게
	const float MinAccel = 2200.0f;
	float AccelMag = Gravity * (1.0f - N.Z) * SlopeSlideScale;
	AccelMag = FMath::Max(AccelMag, MinAccel);

	Velocity.X += SlideDir.X * AccelMag * DeltaTime;
	Velocity.Y += SlideDir.Y * AccelMag * DeltaTime;
}

bool USkullyMovementComponent::ApplySlopeSlide(float DeltaTime)
{
	CachedSlopeAmount = 0.0f;

	// 기본 전제: Grounded + 입력 없음일 때만 정지 후 굴러떨어짐을 평가한다.
	if (MovementMode != ESkullyMovementMode::Grounded)
	{
		bIsSlopeSliding = false;
		return false;
	}

	// 입력이 있으면(특히 사이드 이동 포함) 정지 슬라이드(미끄러짐) 로직을 끈다
	const FVector PendingInput = GetPendingInputVector();
	const float InputDeadZone = 0.1f;
	const bool bHasInputNow = PendingInput.SizeSquared() > FMath::Square(InputDeadZone);

	if (bHasInputNow == true)
	{
		bIsSlopeSliding = false; // 슬라이드 상태도 끊어줌(잔상 방지)
		PendingSlopeSlideAccel2D = FVector::ZeroVector;
		bSlopeSlideThisFrame = false;
		return false;
	}

	FVector UseNormal = CachedFloorNormal;
	// Grounded이고 바닥 히트가 유효하면 슬라이드 계산용 노멀은 히트 노멀 우선
	if (MovementMode == ESkullyMovementMode::Grounded && CurrentFloorHit.bBlockingHit == true)
	{
		UseNormal = CurrentFloorHit.ImpactNormal;
	}
	// 불안정 바닥에서는 CachedFloorNormal이 튀는 프레임이 있으므로, 슬라이드에 사용할 노멀을 안정화
	// 기존의 엣지/불안정 처리만 보강용으로 유지
	const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
	const bool bUnstableForSlide = (CachedFloorNormal.Z < UnstableFloorZThreshold) || (NormalDot <
		FloorNormalDotEdgeThreshold);
	if (bUnstableForSlide == true)
	{
		UseNormal = LastFloorNormal;
	}
	UseNormal = UseNormal.GetSafeNormal();

	// 중력의 바닥 평면 성분(경사 아래 가속)
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UseNormal);

	// 엣지/경계 보강(노멀이 애매해서 방향(AlongPlane)이 거의 0이면 주변 샘플로 downhill 방향을 추정)
	const float MinSlopeForSamplesZ = 0.98f;
	if (AlongPlane.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		FVector DownhillDir;
		if (UseNormal.Z < MinSlopeForSamplesZ &&
			TryGetDownhillDirFromSamples(GetWorld(), UpdatedComponent->GetComponentLocation(), DownhillSampleDistance,
			                             GroundLineTraceDistance + 50.0f, GetOwner(), DownhillDir))
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
	CachedSlopeAmount = SlopeAmount;
	// 커브: 지수 < 1이면 완만한 경사도 좀 더 잘 미끄러짐
	const float Curve = FMath::Pow(SlopeAmount, 0.25f);
	// 가속 크기(게임 감성)
	float SlideAccelMag = Gravity * Curve * SlopeSlideScale;
	// 최소 가속 보장(너무 느린 체감 방지)
	const float MinSlideAccel = FMath::Lerp(1600.0f, 4200.0f, SlopeAmount);
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

	const float CosTheta = FMath::Clamp(UseNormal.Z, 0.0f, 1.0f);
	const float SinTheta = FMath::Sqrt(FMath::Max(0.0f, 1.0f - CosTheta * CosTheta));
	const float TanTheta = (CosTheta > KINDA_SMALL_NUMBER) ? (SinTheta / CosTheta) : BIG_NUMBER;

	const float StaticMu = StaticFrictionMu;
	const bool bShouldStartSliding = TanTheta > StaticMu;

	// 시작(Static friction): 아직 슬라이딩 중이 아니면 정지 마찰을 이겨야 시작
	if (bIsSlopeSliding == false)
	{
		if (bShouldStartSliding == false)
		{
			return false;
		}

		bIsSlopeSliding = true;
	}

	// 슬라이드 가속을 Move()에서 더할 수 있도록 저장
	PendingSlopeSlideAccel2D = FVector(SlideDir.X, SlideDir.Y, 0.0f) * SlideAccelMag;
	bSlopeSlideThisFrame = true;
	return true;
}

void USkullyMovementComponent::ApplyFriction(float DeltaTime, float GroundedFriction)
{
	const bool bGrounded = MovementMode == ESkullyMovementMode::Grounded;
	// 슬라이드 상태 판단(이번 프레임 슬라이드 가속이 있거나, 슬라이딩 플래그가 켜진 경우)
	const bool bSlidingNow = bGrounded == true && (bIsSlopeSliding == true || bSlopeSlideThisFrame == true ||
		bSlopeSlideAppliedThisFrame == true);

	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	if (HorizontalVelocity.IsNearlyZero() == true)
	{
		return;
	}

	if (bSlidingNow == true)
	{
		// 슬라이딩은 속도 비례 감쇠(곱 감쇠)가 체감이 좋음
		// SlidingFriction을 초당 감쇠율처럼 사용
		const float Damping = FMath::Lerp(0.06f, 0.02f, CachedSlopeAmount);
		const float Factor = FMath::Exp(-Damping * DeltaTime);
		HorizontalVelocity *= Factor;
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

void USkullyMovementComponent::Move(float DeltaTime)
{
	// 1. 변수 초기화
	const FVector Start = UpdatedComponent->GetComponentLocation(); // 이동 처리 전 시작 위치
	// 이번 프레임 벽 입력/정보(폭발 방지용)
	float PressWallAlpha = 0.0f; // 0~1 (벽으로 누르는 세기만 추출한 값)
	float InputIntoWall = 0.0f; // -1~1 (입력 방향이 벽 노멀과 이루는 정도: 이탈/박음 판정용)
	bool bTryingToLeaveWall = false;

	// 2. 입력 소비
	const FVector Input = ConsumeMovementInput(); // 입력 벡터
	const bool bHasInput = Input.IsNearlyZero() == false; // 입력이 있는지
	const FVector InputDir = bHasInput == true ? Input.GetSafeNormal() : FVector::ZeroVector; // 입력 방향

	// 3. 가파른 경사면 입력 제약 판정
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle)); // 걸을 수 있는 경사면 기준 값(MaxSlopeAngle을 노멀.Z로 바꾼 값) 
	// 현재 바닥 노멀: CachedFloorNormal이 아닌 CurrentFloorHit.ImpactNormal을 우선->조작 방지는 실제 면 노멀이 중요
	FVector ControlFloorN = (CurrentFloorHit.bBlockingHit == true ? CurrentFloorHit.ImpactNormal : CachedFloorNormal).GetSafeNormal();
	const bool bTooSteepNow = MovementMode == ESkullyMovementMode::Grounded && ControlFloorN.Z < WalkableZ; // 가파른 경사인지 판정

	// 5. 목표 속도(TargetVelocity2D) 만들기: 바닥 투영 + 너무 가파르면 업힐 제거 + 옆 이동 감쇠
	FVector TargetVelocity2D = FVector::ZeroVector;
	if (bHasInput == true)
	{
		// 입력 방향을 바닥 평면으로 투영
		FVector InputOnPlane = FVector::VectorPlaneProject(InputDir, ControlFloorN);
		InputOnPlane.Z = 0.0f;
		if (InputOnPlane.IsNearlyZero() == false)
		{
			InputOnPlane.Normalize();
			
			const FVector Downhill2D = GetDownhillDir2D(ControlFloorN);
			const FVector Uphill2D = -Downhill2D;
            	
			// 가파른 경사라면
			if (bTooSteepNow == true && Downhill2D.IsNearlyZero() == false)
			{
				// 업힐 성분 제거: 등반 불가
				const float UphillAmount = FVector::DotProduct(InputOnPlane, Uphill2D);
				if (UphillAmount > 0.0f)
				{
					InputOnPlane -= Uphill2D * UphillAmount;
				}
				
				// 업힐로 강하게 누를수록(벽에 파고드는 입력) 옆 이동을 감쇠
				const float PushIntoSlope = FMath::Clamp(UphillAmount, 0.0f, 1.0f);
				const float Denom = FMath::Max(KINDA_SMALL_NUMBER, UphillPushFullThreshold - UphillPushStartThreshold);
				const float LateralReductionAlpha = FMath::Clamp((PushIntoSlope - UphillPushStartThreshold) / Denom, 0.0f, 1.0f);
				const float LateralScale = 1.0f - LateralReductionAlpha;
				InputOnPlane *= LateralScale;
			}
			
			InputOnPlane.Z = 0.0f;
			InputOnPlane = InputOnPlane.GetSafeNormal();
			// 찌꺼기 입력 벡터 정리
			if (InputOnPlane.SizeSquared() < FMath::Square(MinProjectedInputStrength))
			{
				InputOnPlane = FVector::ZeroVector;
			}
			// 유효한 입력 벡터만 목표 속도로 처리
			if (InputOnPlane.IsNearlyZero() == false)
			{
				TargetVelocity2D = InputOnPlane * MaxSpeed;
			}
		}
	}

	// 6. 속도 적분: 입력 가속 + 슬라이드 가속 + 속도 상한
	FVector CurrentVelocity2D(Velocity.X, Velocity.Y, 0.0f);
	// 6-1. 입력 가속
	if (TargetVelocity2D.IsNearlyZero() == false)
	{
		CurrentVelocity2D = FMath::VInterpConstantTo(CurrentVelocity2D, TargetVelocity2D, DeltaTime, Acceleration);
	}
	// 6-2. 슬라이드 가속
	if (MovementMode == ESkullyMovementMode::Grounded && bSlopeSlideThisFrame == true)
	{
		CurrentVelocity2D += PendingSlopeSlideAccel2D * DeltaTime;
	}
	// 6-3. 속도 상한(미끄러질 때 상한 상승)
	const bool bClampAsSlope = MovementMode == ESkullyMovementMode::Grounded && bSlopeSlideThisFrame == true;
	CurrentVelocity2D = CurrentVelocity2D.GetClampedToMaxSize(bClampAsSlope ? MaxSlopeSlideSpeed : MaxSpeed);
	Velocity.X = CurrentVelocity2D.X;
	Velocity.Y = CurrentVelocity2D.Y;

	// 7. 최종 이동 벡터
	FVector MoveDelta;
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		MoveDelta = FVector(Velocity.X, Velocity.Y, 0.0f) * DeltaTime;
	}
	else
	{
		MoveDelta = Velocity * DeltaTime;
	}

	// 8. Grounded일 때 최종 이동 벡터 정제(투영 + 노멀 튐 보정 + 업힘 제거)
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		const float Speed2D = Velocity2D.Size();
		const FVector VelocityDir2D = (Speed2D > KINDA_SMALL_NUMBER) ? (Velocity2D / Speed2D) : FVector::ZeroVector;
		
		FVector AdjustedMove = FVector::ZeroVector; // 최종 이동 벡터 조정 변수
		
		const float NormalDot = FVector::DotProduct(CachedFloorNormal, LastFloorNormal); // 이전 프레임과 현재 프레임의 바닥 노멀 변화량
		const bool bUnstableFloor = CachedFloorNormal.Z < UnstableFloorZThreshold || // 현재 바닥 노멀이 평평하지 한지 
									NormalDot < FloorNormalDotEdgeThreshold; // 이전 프레임과 현제 프레임의 바닥 노멀이 급격하게 달라지는지
		
		// 8-1. 불안정한 바닥이면
		if (bUnstableFloor == true)
		{
			const FVector FallbackDir = bHasInput == true ? InputDir : VelocityDir2D;
			if (FallbackDir.IsNearlyZero() == false)
			{
				// 입력 기반으로 강제 이동
				AdjustedMove = FallbackDir * Speed2D * DeltaTime;
			}
			// bHasInput == false && VelocityDir2D == ZeroVector: 입력이 없고 속도가 0일 때(정지 상태)
			else
			{
				// 입력도 속도도 없지만 MoveDelta가 0이 아닌 경우: SlideAlongSurface, StickDist, 속도 동기화/투영 보정 후 미세하게 남은 Velocity
				// 불안정한 바닥(엣지/폴리곤 경계)은 약간만 이동해도 해결 가능
				// fallback 방향이 없다면 최소한의 조치(투영)
				AdjustedMove = MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER ? FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal) : FVector::ZeroVector;
			}
		}
		// 8-2. 안정한 바닥이면
		else
		{
			AdjustedMove = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal); // 바닥 평면으로 투영
			// 투영 결과가 너무 작으면
			if (AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
			{
				const FVector FallbackDir = bHasInput ? InputDir : VelocityDir2D;
				AdjustedMove = FallbackDir * Speed2D * DeltaTime;
				
				const bool bAllowMinMoveGuarantee = bHasInput == true && bTooSteepNow == false; // 입력이 있고 가파르지 않는 경사일 경우 
				// 최소 이동 보장: 가속 초기, Speed2D가 너무 작을 경우
				if (bAllowMinMoveGuarantee == true && AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
				{
					AdjustedMove = FVector(FallbackDir.X, FallbackDir.Y, 0.0f) * MinProjectedMoveCm;
				}
			}
		}

		MoveDelta = AdjustedMove;

		// 8-3. 최종 이동 결과(MoveDelta) 기준 업힐 제거
		if (bTooSteepNow == true)
		{
			const FVector GravityDir = -FVector::UpVector;
			FVector Downhill3D = FVector::VectorPlaneProject(GravityDir, ControlFloorN).GetSafeNormal();
			FVector Uphill3D = -Downhill3D;

			if (Uphill3D.IsNearlyZero() == false)
			{
				const float UphillAmount = FVector::DotProduct(MoveDelta, Uphill3D);
				if (UphillAmount > 0.0f)
				{
					MoveDelta -= Uphill3D * UphillAmount;
				}
			}
			// 찌꺼기 최종 이동 벡터 정리
			if (MoveDelta.SizeSquared() < KINDA_SMALL_NUMBER)
			{
				MoveDelta = FVector::ZeroVector;
			}
		}
	}

	// 9. 실제 이동
	FHitResult Hit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	// 이동 후 접지: Grounded 상태에서 떠버리는 프레임을 없앰(경사, 범프, 경계 등)
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		FHitResult StickHit;
		SafeMoveUpdatedComponent(-FVector::UpVector * StickDist, UpdatedComponent->GetComponentQuat(), true, StickHit);
	}
	
	bool bFloorBumpThisFrame = false; // 범프에 충돌했는지
	bool bHitWallThisFrame = false; // 벽에 충돌했는지
	
	// 10. 충돌 처리
	if (Hit.bBlockingHit == true)
	{
		// 10-1. 변수 초기화
		FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		const float Speed2D = Velocity2D.Size();
		FVector WallN2D = FVector(Hit.Normal.X, Hit.Normal.Y, 0.0f).GetSafeNormal(); // 벽 노멀
		const bool bHasWallN2D = WallN2D.IsNearlyZero() == false; // 벽 노멀 유효 플래그
		float IntoWallVel2D = 0.0f; // 벽으로 파고드는 속도(> 0: 파고듦, == 0: 스침, < 0: 떨어짐)
		if (bHasWallN2D == true && Velocity2D.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			IntoWallVel2D = FVector::DotProduct(Velocity2D, WallN2D);
		}
		const bool bLooksLikeFloor = Hit.Normal.Z >= WalkableZ; // 충돌면이 걸을 수 있는 바닥인지
		// 정면 충돌 비율(1: 정면 돌진, 0: 스침, < 0: 떨어짐)
		const float IntoRatio = Speed2D > KINDA_SMALL_NUMBER ? IntoWallVel2D / Speed2D : 0.0f;
		const bool bHeadOnIntoWall = IntoRatio > WallHeadOnRatioThreshold; // 정면 충돌 플래그
		// 최종 벽 판정: 바닥 아님 + 벽 노멀 유효 + 충돌 면이 수직에 가까움 + 충분한 세기로 박음 + 정면 돌진 
		const bool bTreatAsWall = bLooksLikeFloor == false && bHasWallN2D == true && 
								  Hit.Normal.Z < WallNormalZMax && IntoWallVel2D > MinIntoWallSpeed && bHeadOnIntoWall == true;
		
		// 10-2. 바닥인 경우(턱, 범프): 속도를 죽이지 말고, 위치만 슬라이드로 정리하고, 속도는 접선으로만 살짝 정리
		if (bLooksLikeFloor == true)
		{
			bFloorBumpThisFrame = true; // 범프 플래그
			
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit); // 위치 보정
			
			FVector Projected = FVector::VectorPlaneProject(Velocity2D, Hit.Normal); // 속도 처리: 바닥면 접선 방향으로 투영하여 최대한 속도 크기 유지
			Projected.Z = 0.0f;
			
			// 범프에 걸린 프레임에 속도 방향이 노멀 튐 때문에 확 꺾여서 굴림이 끊기는 문제를 막는 방향 회전 제한기
			if (Speed2D > KINDA_SMALL_NUMBER && Projected.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				const FVector OldDir = Velocity2D / Speed2D; // 기존 방향
				FVector NewDir = Projected.GetSafeNormal(); // 새 방향
				const float MaxTurnRad = FMath::DegreesToRadians(MaxTurnDeg); // 회전 한계 값
				const float Dot = FMath::Clamp(FVector::DotProduct(OldDir, NewDir), -1.0f, 1.0f); // 1: 같은 방향, 0: 직각, -1: 반대 방향
				const float Angle = FMath::Acos(Dot); // 각도 계산

				if (Angle > MaxTurnRad)
				{
					const float Alpha = MaxTurnRad / Angle;
					NewDir = FMath::Lerp(OldDir, NewDir, Alpha).GetSafeNormal(); // 완만한 회전
				}

				const FVector Final = NewDir * Speed2D; // 속도는 유지하고 방향만 갱신
				Velocity.X = Final.X;
				Velocity.Y = Final.Y;
			}
		}
		// 10-3. 벽인 경우: 옆으로 미끄러지며 에너지가 보존되거나, 자석처럼 붙는 느낌이 나지 않게, 벽 성분을 죽이고 접선을 상황에 따라 감쇠
		else if (bTreatAsWall == true)
		{
			const FVector InputDir2D = FVector(InputDir.X, InputDir.Y, 0.0f); // 입력 정보
			
			bHitWallThisFrame = true; // 벽 충돌 플래그
			
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit); // 위치 보정
			
			InputIntoWall = 0.0f; // 입력 방향이 벽으로 향하는 정도(> 0: 파고듦, < 0: 떨어짐)
			PressWallAlpha = 0.0f; // 벽을 미는 정도
			bTryingToLeaveWall = false; // 이탈 시도
			if (bHasInput == true && bHasWallN2D == true)
			{
				InputIntoWall = FVector::DotProduct(InputDir2D, WallN2D); // 입력 방향이 벽 노멀 방향으로 얼마나 향하는지
				PressWallAlpha = FMath::Clamp(InputIntoWall, 0.0f, 1.0f); // 벽 쪽으로 누르는 양만 추출
				bTryingToLeaveWall = InputIntoWall < -0.1f; // 아날로그 스틱 노이즈나 입력 방향이 정확히 반대로 안 나올 수도 있으니 확실히 떨어지려고 누를 때만
			}

			// 속도 정리: 벽으로 파고드는 성분 제거 + 벽 누를수록 접선 감쇠
			if (bHasWallN2D == true && Velocity2D.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				const float IntoWallVel = FVector::DotProduct(Velocity2D, WallN2D); // 벽으로 파고드는 성분
				const bool bSeparatingFromWall = IntoWallVel <= 0.0f; // 이미 떨어지는 중인지
				const bool bLeavingWallNow = bTryingToLeaveWall == true || bSeparatingFromWall == true; // 벽에 붙어있는지

				// 벽에 붙어있는 있다면
				if (bLeavingWallNow == false)
				{
					// 벽으로 파고드는 성분 제거/감쇠
					if (IntoWallVel > 0.f)
					{
						Velocity2D -= WallN2D * (IntoWallVel * WallNormalKill);
					}
					// 벽을 누를수록 접선(옆) 성분 감쇠
					if (PressWallAlpha > 0.0f)
					{
						// 벽에 더 세게 박을수록(1에 가까울수록) 접선 속도를 더 많이 죽임
						const float TangentialFactor = FMath::Exp(-PressWallAlpha * MaxTangentialKill * 60.0f * DeltaTime);
						Velocity2D *= TangentialFactor;
					}
					// 이탈 입력이 아닐 때만 추가 감쇠(벽을 타고 흘러가는 에너지 자체를 줄이는 기본 댐핑)
					if (bTryingToLeaveWall == false)
					{
						const float Factor = FMath::Clamp(1.0f - WallSlideDamping * DeltaTime, 0.0f, 1.0f);
						Velocity2D *= Factor;
					}

					Velocity.X = Velocity2D.X;
					Velocity.Y = Velocity2D.Y;
				}
				// 벽에서 나가려는 상황
				else
				{
					if (IntoWallVel > 0.f)
					{
						Velocity2D -= WallN2D * IntoWallVel; // 벽으로 박는 성분 완전 제거
					}

					Velocity.X = Velocity2D.X;
					Velocity.Y = Velocity2D.Y;
				}
			}
			
			// Grounded 2-pass(엣지/코너에서 이동 소실 복구)
			if (MovementMode == ESkullyMovementMode::Grounded) // 공중에서 2-pass는 바닥 재투영이 될 수 있음
			{
				if (bSlopeSlideAppliedThisFrame == false) // 경사 슬라이드로 가속이 들어가 프레임에서는 이동이 2번 먹어서 거리 폭발이 생길 수 있음
				{
					if (bTryingToLeaveWall == false) // 이탈 시 재투영을 해버리면 벽에서 빠지려는 순간 다시 붙는 느낌이 날 수 있음
					{
						const FVector FloorSlide = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);
						if (FloorSlide.IsNearlyZero() == false)
						{
							FHitResult FloorHit;
							SafeMoveUpdatedComponent(FloorSlide * 0.5f, UpdatedComponent->GetComponentQuat(), true, FloorHit); // 0.5는 과보정/튐 방지 안전 스탭
						}
					}
				}
			}
		}
		// 애매한 면(엣지/모서리/급격한 노멀 변화)
		else
		{
			// 여기서 벽 로직을 걸면 울퉁불퉁에서 속도 끊김이 다시 생기기 쉬움
			// -> 기본은 바닥 범프처럼 취급(위치 보정 + 속도 투영만)
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);

			if (MovementMode == ESkullyMovementMode::Grounded)
			{
				// 애매한 면에서 옆면 노멀로 속도 꺾지 말고, 바닥 기준으로만 정리
				Velocity2D = FVector::VectorPlaneProject(Velocity2D, ControlFloorN);
			}
			else
			{
				Velocity2D = FVector::VectorPlaneProject(Velocity2D, Hit.Normal);
			}
			Velocity2D.Z = 0.0f;

			Velocity.X = Velocity2D.X;
			Velocity.Y = Velocity2D.Y;

			// 이 프레임은 바닥 범프처럼 취급해서 아래 추가 감쇠/동기화 트리거 피하기
			bFloorBumpThisFrame = MovementMode == ESkullyMovementMode::Grounded;
		}
	}

	// 경사면 좌우 감쇠는 벽 처리 이후에 한다.
	// 그래야 bHitWallThisFrame이 true인 프레임에서는 스킵되어 떨림/붙었다 떨어짐이 줄어든다.
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		const bool bApplySlopeLateralDamping = bHasInput == false && (bIsSlopeSliding == true || bSlopeSlideThisFrame ==
			true/* || bTooSteepNow == true*/) && bHitWallThisFrame == false && bFloorBumpThisFrame == false;

		if (bApplySlopeLateralDamping == true)
		{
			const FVector FloorN = ControlFloorN.GetSafeNormal();
			const FVector GravityDir(0.0f, 0.0f, -1.0f);

			// 경사 아래 방향(바닥 평면 뒤)
			FVector Downhill3D = FVector::VectorPlaneProject(GravityDir, FloorN).GetSafeNormal();
			Downhill3D.Z = 0.0f;
			Downhill3D = Downhill3D.GetSafeNormal();

			if (Downhill3D.IsNearlyZero() == false)
			{
				FVector V2D(Velocity.X, Velocity.Y, 0.0f);

				// 슬라이드(Downhill) 방향 성분
				const float Along = FVector::DotProduct(V2D, Downhill3D);
				const FVector AlongV = Downhill3D * Along;

				// 접선(좌우) 성분
				FVector LateralV = V2D - AlongV;

				// 벽 감쇠처럼: 접선 성분만 댐핑
				const float LateralDamping = 10.0f;
				const float Factor = FMath::Clamp(1.0f - LateralDamping * DeltaTime, 0.0f, 1.0f);
				LateralV *= Factor;

				V2D = AlongV + LateralV;

				Velocity.X = V2D.X;
				Velocity.Y = V2D.Y;
			}
		}
	}

	// 실제 이동량 기록(비주얼 롤링용)
	const FVector End = UpdatedComponent->GetComponentLocation();
	LastActualDelta = End - Start;

	// 폭발 방지: Grounded에서 제약이 걸린 프레임이면 속도를 실제 이동량과 동기화
	if (DeltaTime > KINDA_SMALL_NUMBER && MovementMode == ESkullyMovementMode::Grounded)
	{
		// 제약이 있다고 보는 조건들: 너무 가파름/슬라이드/벽 히트
		const bool bConstrainedFrame =
			(bHitWallThisFrame == true && PressWallAlpha > 0.05f && bTryingToLeaveWall == false) ||
			(bTooSteepNow == true && bHasInput == true);

		if (bConstrainedFrame == true)
		{
			FVector ActualVel2D = LastActualDelta / DeltaTime;
			ActualVel2D.Z = 0.0f;

			// 너무 미세하게 막힌 프레임(계단/턱/미세 충돌)에서 속도 0으로 박히는 걸 방지
			if (ActualVel2D.SizeSquared() > FMath::Square(5.0f))
			{
				// 너무 딱딱하게 싫으면 여기 값을 0.5~0.8로 (부드럽게 따라가게)
				const float SyncAlpha = 0.35f;
				const FVector CurVel2D(Velocity.X, Velocity.Y, 0.0f);
				const FVector NewVel2D = FMath::Lerp(CurVel2D, ActualVel2D, SyncAlpha);

				Velocity.X = NewVel2D.X;
				Velocity.Y = NewVel2D.Y;
			}
		}
	}
}

FVector USkullyMovementComponent::ConsumeMovementInput()
{
	// 입력으로 쌓인 벡터(PendingInputVector)을 소비(가져오고 초기화)
	FVector Input = ConsumeInputVector();

	if (Input.IsNearlyZero() == false)
	{
		// 크기 정규화(크기를 1로 클램프하여 대각선 입력이 과속되지 않게 함)
		Input = Input.GetClampedToMaxSize(1.0f);
	}

	return Input;
}

FVector USkullyMovementComponent::GetDownhillDir2D(const FVector& FloorN)
{
	const FVector GravityVec = -FVector::UpVector;
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVec, FloorN);
	AlongPlane.Z = 0.0f;
	return AlongPlane.GetSafeNormal();
}

void USkullyMovementComponent::CheckGround(float DeltaTime)
{
	// 1. 점프 직후 일정 시간 바닥 판정 무시
	if (JumpIgnoreGroundRemaining > 0.0f)
	{
		JumpIgnoreGroundRemaining -= DeltaTime;
		bOnUnwalkableSlope = false; // 이전 프레임에 true였다면, 점프 직후에도 그대로 남아서 ApplyUnwalkableSlide가 적용될 위험이 있음
		CurrentFloorHit = FHitResult(); // 공중으로 들어간 순간 더 이상 유효한 바닥 히트가 아니므로 비워둠

		return;
	}

	// 2. 관련 변수 초기화
	FHitResult Hit;
	const bool bHitGround = SweepGround(Hit); // 바닥 Sweep 정보
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle)); // 걸을 수 있는 경사면 기준 값(MaxSlopeAngle을 노멀.Z로 바꾼 값) 
	bOnUnwalkableSlope = false;

	// 3. 유효 바닥 후보 처리
	if (bHitGround == true && Hit.bBlockingHit == true && Hit.Distance <= MaxGroundDistance)
	{
		CurrentFloorHit = Hit;
		const float HitZ = Hit.ImpactNormal.Z; // 바닥 노멀의 Z(1: 평지 ~ 0: 벽)

		// 3-1. 걸을 수 있는 바닥
		if (HitZ >= WalkableZ)
		{
			const bool bIsSlope = HitZ < FlatGroundZThreshold; // 경사 판정

			// 평지
			if (bIsSlope == false)
			{
				LastFloorNormal = FVector::UpVector;
				CachedFloorNormal = FVector::UpVector;
				bIsSlopeSliding = false; // 미끄러짐 상태 끔
				
				// 평지 착지 스냅
				if (MovementMode == ESkullyMovementMode::Falling && Velocity.Z <= 0.0f)
				{
					SnapToGround(Hit);
				}
			}
			// 경사
			else
			{
				const FVector Prev = LastFloorNormal.IsNearlyZero() == true ? Hit.ImpactNormal : LastFloorNormal;
				// 이전 프레임의 바닥 노멀과 현재 감지된 바닥 노멀을 보간하여 노멀 튐(엣지, 폴리곤 경계 등) 현상에 대한 오류 값 방지 
				CachedFloorNormal = FMath::VInterpNormalRotationTo(Prev, Hit.ImpactNormal, DeltaTime, FloorNormalInterpSpeed);
				LastFloorNormal = CachedFloorNormal;
			}
			
			MovementMode = ESkullyMovementMode::Grounded;
			
			return;
		}

		// 3-2. 바닥 감지는 됐지만 걸을 수 없음(가파른 경사면)
		if (HitZ > MinSlopeZForSlide)
		{
			bOnUnwalkableSlope = true; // ApplyUnwalkableSlide 함수를 탈 수 있도록 설정
			UnwalkableNormal = Hit.ImpactNormal.GetSafeNormal(); // ApplyUnwalkableSlide 함수에서 사용할 바닥 노멀
			LastFloorNormal = CachedFloorNormal;
			CachedFloorNormal = UnwalkableNormal;
			MovementMode = ESkullyMovementMode::Falling;
			
			return;
		}

		// 3-3. Grace 판정
		// Grounded 상황에서 노말이 잠깐 튄 것 같을 때 관용 처리(예외 상황)
		const float GraceZ = WalkableZ - GroundGraceZOffset; // 걸을 수 있는 경사면 관용 값
		if (MovementMode == ESkullyMovementMode::Grounded && HitZ >= GraceZ)
		{
			LastFloorNormal = CachedFloorNormal;
			// 예외 상황이라 바닥 노멀이 애매한 프레임일 가능성이 높아 최소한의 처리만(보간X)
			CachedFloorNormal = Hit.ImpactNormal.GetSafeNormal();   
			
			return;
		}
	}
	
	// 4. 보조 바닥 판정
	// 스윕이 실패하거나 노멀이 애매한 경우(절벽 가장자리, 꼭짓점, 범프, 고속 이동 + 프레임 오차)
	if (TryConfirmGroundByLineTrace(WalkableZ) == true)
	{
		return;
	}

	MovementMode = ESkullyMovementMode::Falling;
	CurrentFloorHit = FHitResult();
	bOnUnwalkableSlope = false;
}

bool USkullyMovementComponent::SweepGround(FHitResult& OutHit) const
{
	// SphereComponent를 기반으로 함
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return false;
	}

	const float Radius = Sphere->GetScaledSphereRadius();
	const FVector Start = UpdatedComponent->GetComponentLocation(); // SphereComonent 중심
	const FVector End = Start - FVector::UpVector * (Radius + GroundCheckDistance); // 반지름 + GroundCheckDistance

	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
	Params.AddIgnoredActor(GetOwner()); // 자기 자신은 무시
	Params.bReturnFaceIndex = true; // FaceIndex를 받게 설정
	Params.bTraceComplex = true; // 삼각형 기반(Complex)으로 받게 설정

	// Start 지점에서 End 지점까지 Radius 반지름의 구를 스윕(지면과의 접촉/거리/노멀 등을 얻음)
	return GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Visibility,
	                                        FCollisionShape::MakeSphere(Radius), Params);
}

void USkullyMovementComponent::SnapToGround(const FHitResult& Hit) const
{
	// SphereComponent를 기반으로 함
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return;
	}

	// ImpactPoint에 구 반지름만큼 Impace의 노멀로 올려서 구가 바닥에 딱 얹히게 함->파고듦과 떨림을 완화하여 안정성 확보
	const float Radius = Sphere->GetScaledSphereRadius();
	const FVector TargetLocation = Hit.ImpactPoint + Hit.ImpactNormal * Radius;

	UpdatedComponent->SetWorldLocation(TargetLocation);
}

bool USkullyMovementComponent::TryConfirmGroundByLineTrace(float WalkableZ)
{
	if (UpdatedComponent == nullptr || GetWorld() == nullptr)
	{
		return false;
	}
	
	FHitResult LineHit;
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start - FVector::UpVector * GroundLineTraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyGroundSweep), false);
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnFaceIndex = true;
	Params.bTraceComplex = true;

	// 라인 트레이스 성공
	if (GetWorld()->LineTraceSingleByChannel(LineHit, Start, End, ECC_Visibility, Params) == false)
	{
		return false;
	}
	// 충돌이 없으면
	if (LineHit.bBlockingHit == false)
	{
		return false;
	}
	// 걸을 수 없는 바닥이면
	if (LineHit.ImpactNormal.Z < WalkableZ)
	{
		return false;
	}
	// 지면 판정 거리보다 길면
	if (LineHit.Distance > MaxGroundDistance)
	{
		return false;
	}

	CurrentFloorHit = LineHit;
	LastFloorNormal = CachedFloorNormal;
	CachedFloorNormal = LineHit.ImpactNormal.GetSafeNormal();
	MovementMode = ESkullyMovementMode::Grounded;
	bOnUnwalkableSlope = false;
	
	return true;
}

void USkullyMovementComponent::UpdateMotionState()
{
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
	CurrentSpeed2D = Velocity2D.Size();

	CurrentMoveDir2D = (CurrentSpeed2D > KINDA_SMALL_NUMBER) ? Velocity2D / CurrentSpeed2D : FVector::ZeroVector;
}

void USkullyMovementComponent::ApplyVisualRoll(const FVector& ActualDelta) const
{
	// 구르기 연출 플래그
	if (bRollVisualOnMove == false)
	{
		return;
	}
	// 회전시킬 컴포넌트
	USceneComponent* Target = VisualComponent ? VisualComponent : GetOwner()->GetRootComponent();
	if (Target == nullptr)
	{
		return;
	}
	// SphereComponent기반이 아니라면
	const USphereComponent* Sphere = Cast<USphereComponent>(UpdatedComponent);
	if (Sphere == nullptr)
	{
		return;
	}
	// SphereComponnet의 반지름이 너무 작다면
	const float Radius = Sphere->GetScaledSphereRadius();
	if (Radius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 1. 구르기용 벡터 추출에 사용할 바닥 벡터의 노멀
	FVector Up = (MovementMode == ESkullyMovementMode::Grounded)
		             ? CachedFloorNormal.GetSafeNormal()
		             : FVector::UpVector;
	if (Up.IsNearlyZero() == true)
	{
		Up = FVector::UpVector;
		Up.Normalize();
	}
	// 2. 최종 이동 벡터을 바닥에 투영->구르기용 벡터 추출
	const FVector RollDelta = FVector::VectorPlaneProject(ActualDelta, Up);
	const float Dist = RollDelta.Size(); // 구르기용 벡터 크기
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector Dir = RollDelta / Dist; // 구르기용 벡터 방향
	// 3. 회전축 추출: 외적은 두 방향 벡터 사이의 수직
	const FVector Axis = FVector::CrossProduct(Up, Dir).GetSafeNormal();
	if (Axis.IsNearlyZero() == true)
	{
		return;
	}
	// 4. 원에서 호의 길이(굴러간 거리) = 반지름 * 회전 각도(라디안)->각도 = 거리 / 반지름
	const float Angle = (Dist / Radius) * RollVisualScale;
	// 5. Axis축을 기준으로 Angle만큼 회전할 쿼터니언 생성
	const FQuat DeltaRot(Axis, Angle);

	Target->AddWorldRotation(DeltaRot);
}

void USkullyMovementComponent::RequestJump()
{
	// 이미 입력 중이면 추가 요청은 무시
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
		Velocity.Z *= JumpReleaseVelocityScale;
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
		// 점프 발동
		Velocity.Z = JumpSpeed;
		MovementMode = ESkullyMovementMode::Falling;
		// 슬라이드/지면 관련 상태 정리
		bIsSlopeSliding = false;
		// 버퍼 소비
		bWantsToJump = false;
		JumpBufferRemaining = 0.0f;
		// 점프 직후 바닥 판정 무시 타이머 시작
		JumpIgnoreGroundRemaining = JumpIgnoreGroundTime;
	}
}

bool USkullyMovementComponent::CanJump() const
{
	return MovementMode == ESkullyMovementMode::Grounded;
}

bool USkullyMovementComponent::TryStartJumpFromBuffer()
{
	// 점프 요청이 없으면
	if (bWantsToJump == false)
	{
		return false;
	}
	// 버퍼가 만료됐으면
	if (JumpBufferRemaining <= 0.0f)
	{
		bWantsToJump = false;
		return false;
	}
	// 점프가 가능한 상태면
	if (CanJump() == false)
	{
		return false;
	}

	// 점프 발동
	Velocity.Z = JumpSpeed;
	MovementMode = ESkullyMovementMode::Falling;
	// 슬라이드/지면 관련 상태 정리
	bIsSlopeSliding = false;
	// 버퍼 소비
	bWantsToJump = false;
	JumpBufferRemaining = 0.0f;
	// 점프 직후 바닥 판정 무시 타이머 시작
	JumpIgnoreGroundRemaining = JumpIgnoreGroundTime;

	return true;
}

void USkullyMovementComponent::UpdateJumpBufferTimer(float DeltaTime)
{
	// 버퍼가 켜져있지 않으면
	if (JumpBufferRemaining <= 0.0f)
	{
		return;
	}

	JumpBufferRemaining -= DeltaTime;

	// 만료 처리
	if (JumpBufferRemaining <= 0.0f)
	{
		JumpBufferRemaining = 0.0f;
		bWantsToJump = false; // 더 이상 점프 요청을 유지하지 않음
	}
}
