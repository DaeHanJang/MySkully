#include "Skully/SkullyMovementComponent.h"

#include "Components/SphereComponent.h"

namespace
{
	// 노멀이 튀어서 downhill 방향이 0으로 붕괴할 때 샘플링
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
		
		float BaseZ = Origin.Z; // 기준값
		float BestZ = BaseZ; // 결과값
		FVector BestDir = FVector::ZeroVector; // 결과값 방향

		// 샘플링 방향(전/우/후/좌)
		const FVector Dirs[4] = { FVector::ForwardVector, FVector::RightVector, -FVector::ForwardVector, -FVector::RightVector };
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
	bComputedSlopeSlide = false;

	/********************점프********************/
	UpdateJumpBufferTimer(DeltaTime);
	TryStartJumpFromBuffer();
	TryConsumeJump();
	/********************중력********************/
	ApplyGravity(DeltaTime);
	/******************미끄러짐*******************/
	ApplyUnwalkableSlide(DeltaTime);
	bAppliedSlopeSlide = ApplySlopeSlide(DeltaTime);
	ApplyFriction(DeltaTime);
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
			const bool bSlidingLike = bSlopeSlideState == true || 
				                      bComputedSlopeSlide == true || 
				                      bAppliedSlopeSlide == true;

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
	
	const FVector UnwalkableNormal = UnwalkableSlopeNormal.GetSafeNormal();
	if (UnwalkableNormal.IsNearlyZero() == true)
	{
		return;
	}

	// 방향 추출
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UnwalkableNormal); // 중력 벡터를 걸을 수 없는 경사면(가파른 경사면) 노멀에 투영
	AlongPlane.Z = 0.0f; // Z 성분을 제거->Z 성분 누적 방지->면을 따라 옆으로 미끄러뜨림(굴러뜨림)
	const FVector SlideDir = AlongPlane.GetSafeNormal(); // 최종 방향 추출
	if (SlideDir.IsNearlyZero() == true)
	{
		return;
	}

	// 크기 추출
	float AccelMag = Gravity * (1.0f - UnwalkableNormal.Z) * UnwalkableSlopeSlideScale; // 경사량 기반 가속 크기
	AccelMag = FMath::Max(AccelMag, UnwalkableSlopeMinAcceleration); // 최소 가속 크기 보장

	Velocity.X += SlideDir.X * AccelMag * DeltaTime;
	Velocity.Y += SlideDir.Y * AccelMag * DeltaTime;
}

bool USkullyMovementComponent::ApplySlopeSlide(float DeltaTime)
{
	CachedSlopeAmount = 0.0f;
	
	// Grounded 상태가 아니면
	if (MovementMode != ESkullyMovementMode::Grounded)
	{
		bSlopeSlideState = false;
		return false;
	}

	// 입력이 있으면
	const FVector PendingInput = GetPendingInputVector();
	const float InputDeadZone = 0.1f;
	const bool bHasInputNow = PendingInput.SizeSquared() > FMath::Square(InputDeadZone);
	if (bHasInputNow == true)
	{
		bSlopeSlideState = false;
		PendingSlopeSlideAccel2D = FVector::ZeroVector;
		bComputedSlopeSlide = false;
		return false;
	}

	// 현재 바닥 노멀: CurrentFloorHit.ImpactNormal 우선->조작/판정에서는 실제 면 노멀이 유리
	FVector UseNormal = (CurrentFloorHit.bBlockingHit == true ? CurrentFloorHit.ImpactNormal : CachedFloorNormal).GetSafeNormal();
	// 현재 바닥 노멀과 이전 프레임 바닥 노멀의 변화량
	const float NormalDot = FVector::DotProduct(UseNormal, LastFloorNormal);
	// 불안정한 경사면 플래그: 바닥 노멀이 가파르거나 이전 프레임 바닥 노멀과의 변화량이 클 경우
	const bool bUnstableForSlide = (UseNormal.Z < UnstableFloorZThreshold) || (NormalDot < UnstableFloorNormalContinuityThreshold);
	if (bUnstableForSlide == true)
	{
		UseNormal = LastFloorNormal;
	}
	UseNormal = UseNormal.GetSafeNormal();

	// 중력 벡터를 바닥 노멀에 투영
	const FVector GravityVector(0.0f, 0.0f, -Gravity);
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVector, UseNormal);
	
	// 투영 결과가 0일 경우(엣지/경계) 샘플링
	if (AlongPlane.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		FVector DownhillDir;
		if (UseNormal.Z < FlatGroundZThreshold &&
			TryGetDownhillDirFromSamples(GetWorld(), UpdatedComponent->GetComponentLocation(), DownhillSampleDistance,
			                             GroundLineTraceDistance + DownhillSampleLinTraceExtraDistance, GetOwner(), DownhillDir))
		{
			AlongPlane = DownhillDir * (Gravity * (1.0f - UseNormal.Z)); // 샘플링 방향 * 경사량 기반 크기
		}
		else
		{
			bSlopeSlideState = false;
			return false;
		}
	}

	// 방향 확정
	FVector SlideDir = AlongPlane.GetSafeNormal();
	if (SlideDir.IsNearlyZero() == true)
	{
		bSlopeSlideState = false;
		return false;
	}

	// 크기 확정
	const float SlopeAmount = FMath::Clamp(1.0f - UseNormal.Z, 0.0f, 1.0f); // 경사 비율: 0(평지)~1(수직)
	CachedSlopeAmount = SlopeAmount;
	const float Curve = FMath::Pow(SlopeAmount, SlopeSlideCurveExponent); // 완만한 경사에서도 미끄러지게 만드는 감성 커브
	float SlideAccelMag = Gravity * Curve * UnwalkableSlopeSlideScale; // 가속 크기
	const float MinSlideAccel = FMath::Lerp(SlopeSlideMinAccelLow, SlopeSlideMinAccelHigh, SlopeAmount); // 경사량에 따라 최소 가속 크기 설정
	SlideAccelMag = FMath::Max(SlideAccelMag, MinSlideAccel); // 최소 가속 크기 보장
	if (SlideAccelMag <= KINDA_SMALL_NUMBER)
	{
		bSlopeSlideState = false;
		return false;
	}

	// 슬라이딩 종료 히스테리시스: 슬라이딩 중일 때 너무 약해지면 종료
	const float StopThreshold = SlopeSlideStopAccel * 0.25f;
	if (bSlopeSlideState == true && SlideAccelMag < StopThreshold)
	{
		bSlopeSlideState = false;
		return false;
	}

	// 슬라이딩 시작 판정
	const float CosTheta = FMath::Clamp(UseNormal.Z, 0.0f, 1.0f);
	const float SinTheta = FMath::Sqrt(FMath::Max(0.0f, 1.0f - CosTheta * CosTheta));
	const float TanTheta = CosTheta > KINDA_SMALL_NUMBER ? SinTheta / CosTheta : BIG_NUMBER; // 경사 기울기
	const bool bShouldStartSliding = TanTheta > StaticFrictionMu; // 정지 마찰 계수보다 클 때 슬라이딩 시작
	if (bSlopeSlideState == false && bShouldStartSliding == false)
	{
		return false;
	}

	PendingSlopeSlideAccel2D = FVector(SlideDir.X, SlideDir.Y, 0.0f) * SlideAccelMag;
	bSlopeSlideState = true;
	bComputedSlopeSlide = true;
	return true;
}

void USkullyMovementComponent::ApplyFriction(float DeltaTime)
{
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	if (HorizontalVelocity.IsNearlyZero() == true)
	{
		return;
	}

	// 슬라이딩 상태 플래그: 슬라이딩 상태이거나 이번 프레임에서 슬라이드 가속 계산을 했거나 슬라이드 가속이 적용되었으면
	const bool bSlidingNow = MovementMode == ESkullyMovementMode::Grounded == true && (bSlopeSlideState == true || bComputedSlopeSlide == true || bAppliedSlopeSlide == true);
	if (bSlidingNow == true)
	{
		const float Damping = FMath::Lerp(0.06f, 0.02f, CachedSlopeAmount); // 이번 프레임 경사량에 따라 보간
		const float Factor = FMath::Exp(-Damping * DeltaTime); // 지수 감쇠: 속도에 비례해서 감쇠
		HorizontalVelocity *= Factor;
	}
	else
	{
		const float Friction = MovementMode == ESkullyMovementMode::Grounded ? GroundFriction : AirFriction;
		const FVector Decel = -HorizontalVelocity.GetSafeNormal() * Friction * DeltaTime; // 선형 감쇠
		// 감속이 현재 속도보다 클 경우->역방향으로 튕기는 것 방지
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
	const FVector Start = UpdatedComponent->GetComponentLocation(); // 이동 처리 전 위치(강제 동기화 및 비주얼 롤링에 쓰일 실제 이동량 변수 계산용)

	/*************** 1. 입력 소비 ***************/
	const FVector Input = ConsumeMovementInput(); // 입력 벡터
	const bool bHasInput = Input.IsNearlyZero() == false; // 입력이 있는지
	const FVector InputDir = bHasInput == true ? Input.GetSafeNormal() : FVector::ZeroVector; // 입력 방향

	/*************** 2. 변수 초기화 ***************/
	// 걸을 수 있는 노멀의 Z
	const float WalkableZ = FMath::Cos(FMath::DegreesToRadians(MaxSlopeAngle)); 
	// 현재 바닥 노멀: CurrentFloorHit.ImpactNormal 우선->조작/판정에서는 실제 면 노멀이 유리
	const FVector GroundFloorNormal = (CurrentFloorHit.bBlockingHit == true ? CurrentFloorHit.ImpactNormal : CachedFloorNormal).GetSafeNormal();
	// Grounded 상태에서 가파른 경사인지->이동 제약
	const bool bTooSteepNow = MovementMode == ESkullyMovementMode::Grounded && GroundFloorNormal.Z < WalkableZ;
	// 충돌 처리 시 범프에 충돌했는지
	bool bHitFloorBumpThisFrame = false;
	// 층돌 처리 시 벽에 충돌했는지
	bool bHitWallThisFrame = false;
	// 벽 충돌 시 파고드는 정도(0~1)
	float PressWallAlpha = 0.0f;
	// 벽 충돌 시 이탈 방향 플래그
	bool bTryingToLeaveWall = false;

	/*************** 3. 목표 속도 ***************/
	FVector TargetVelocity2D = FVector::ZeroVector;
	if (bHasInput == true)
	{
		// 3-1. 입력 방향을 바닥 평면으로 투영
		FVector InputOnPlane = FVector::VectorPlaneProject(InputDir, GroundFloorNormal);
		InputOnPlane.Z = 0.0f;
		
		if (InputOnPlane.IsNearlyZero() == false)
		{
			// 가파른 경사일 때
			if (bTooSteepNow == true)
			{
				FVector Downhill2D = GetDownhillDir(GroundFloorNormal);
				Downhill2D = FVector(Downhill2D.X, Downhill2D.Y, 0.0f).GetSafeNormal();
				const FVector Uphill2D = -Downhill2D;
				
				if (Uphill2D.IsNearlyZero() == false)
				{
					// 3-2. 업힐 성분 제거
					const float UphillDirAmount = FVector::DotProduct(InputOnPlane.GetSafeNormal(), Uphill2D);
					if (UphillDirAmount > 0.0f)
					{
						const float UphillComponent = FVector::DotProduct(InputOnPlane, Uphill2D);
						InputOnPlane -= Uphill2D * UphillComponent;
					}
					// 3-3. 접선(좌우) 성분 감쇠
					const float PushIntoSlope = FMath::Clamp(UphillDirAmount, 0.0f, 1.0f);
					const float Denom = FMath::Max(KINDA_SMALL_NUMBER, UphillPushFullThreshold - UphillPushStartThreshold);
					const float LateralReductionAlpha = FMath::Clamp((PushIntoSlope - UphillPushStartThreshold) / Denom, 0.0f, 1.0f);
					const float LateralScale = 1.0f - LateralReductionAlpha;
					InputOnPlane *= LateralScale;
				}
			}
			
			if (InputOnPlane.SizeSquared() < FMath::Square(MinInputMagnitudeForTargetVelocity2D))
			{
				InputOnPlane = FVector::ZeroVector;
			}
			else
			{
				InputOnPlane = InputOnPlane.GetSafeNormal();
				TargetVelocity2D = InputOnPlane * MaxSpeed;
			}
		}
	}

	/*************** 4. 속도 적분 ***************/
	FVector CurrentVelocity2D(Velocity.X, Velocity.Y, 0.0f);
	// 4-1. 가속(입력이 있을 떄)
	if (TargetVelocity2D.IsNearlyZero() == false)
	{
		CurrentVelocity2D = FMath::VInterpConstantTo(CurrentVelocity2D, TargetVelocity2D, DeltaTime, Acceleration);
	}
	// 4-2. 슬라이드 가속(미끄러지는 중일 때)
	if (MovementMode == ESkullyMovementMode::Grounded && bComputedSlopeSlide == true)
	{
		CurrentVelocity2D += PendingSlopeSlideAccel2D * DeltaTime;
	}
	// 4-3. 속도 상한
	const bool bClampAsSlope = MovementMode == ESkullyMovementMode::Grounded && bComputedSlopeSlide == true;
	CurrentVelocity2D = CurrentVelocity2D.GetClampedToMaxSize(bClampAsSlope ? MaxSlopeSlideSpeed : MaxSpeed);
	Velocity.X = CurrentVelocity2D.X;
	Velocity.Y = CurrentVelocity2D.Y;

	/*************** 5. 기본 이동량 ***************/
	FVector MoveDelta;
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		MoveDelta = FVector(Velocity.X, Velocity.Y, 0.0f) * DeltaTime;
	}
	else
	{
		MoveDelta = Velocity * DeltaTime;
	}
	
	/*************** 6. 기본 이동량 정제 ***************/
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		// 6-1. 변수 초기화
		// 속도 2D
		const FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		// 속도 크기
		const float Speed2D = Velocity2D.Size();
		// 속도 방향
		const FVector VelocityDir2D = Speed2D > KINDA_SMALL_NUMBER ? Velocity2D / Speed2D : FVector::ZeroVector;
		// 기본 이동량 정제 변수
		FVector AdjustedMove = FVector::ZeroVector;
		// 바닥 노멀 변화량
		const float FloorNormalContinuity = FVector::DotProduct(CachedFloorNormal, LastFloorNormal);
		// 안정한 바닥 플래그: 현재 바닥 노멀의 완만함 + 바닥 노멀 변화량의 완만함
		const bool bUnstableFloor = CachedFloorNormal.Z < UnstableFloorZThreshold || 
									FloorNormalContinuity < UnstableFloorNormalContinuityThreshold;
		
		// 6-1. 불안정한 바닥
		if (bUnstableFloor == true)
		{
			const FVector FallbackDir = bHasInput == true ? InputDir : VelocityDir2D;
			// 입력 기반 강제 이동
			if (FallbackDir.IsNearlyZero() == false)
			{
				AdjustedMove = FallbackDir * Speed2D * DeltaTime;
			}
			// 입력도 없고 속도도 없을 때: 충돌/보정으로 남은 MoveDelta를 이용
			else
			{
				AdjustedMove = MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER ? FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal) : FVector::ZeroVector;
			}
		}
		// 6-2. 안정한 바닥
		else
		{
			AdjustedMove = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal); // 바닥 평면으로 투영
			
			// 투영 결과가 작으면
			if (AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
			{
				const FVector FallbackDir = bHasInput == true ? InputDir : VelocityDir2D;
				if (FallbackDir.IsNearlyZero() == false)
				{
					AdjustedMove = FallbackDir * Speed2D * DeltaTime;
					
					// 최소 이동 보장: 입력 초반 속도가 작을 경우
					// 조건: 입력이 있고 가파른 경사가 아닐 때
					const bool bAllowMinMoveGuarantee = bHasInput == true && bTooSteepNow == false; 
					if (bAllowMinMoveGuarantee == true && AdjustedMove.SizeSquared() < FMath::Square(MinProjectedMoveCm))
					{
						AdjustedMove = FVector(FallbackDir.X, FallbackDir.Y, 0.0f) * MinProjectedMoveCm;
					}
				}
				else
				{
					AdjustedMove = MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER ? FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal) : FVector::ZeroVector;
				}
			}
		}

		MoveDelta = AdjustedMove;

		// 6-3. 정제된 기본 이동량 업힐 제거
		if (bTooSteepNow == true)
		{
			const FVector Downhill3D = GetDownhillDir(GroundFloorNormal);
			const FVector Uphill3D = -Downhill3D;

			if (Uphill3D.IsNearlyZero() == false)
			{
				const float UphillAmount = FVector::DotProduct(MoveDelta, Uphill3D);
				if (UphillAmount > 0.0f)
				{
					MoveDelta -= Uphill3D * UphillAmount;
				}
			}
			
			if (MoveDelta.SizeSquared() < FMath::Square(MinProjectedMoveCm))
			{
				MoveDelta = FVector::ZeroVector;
			}
		}
	}

	/*************** 7. 실제 이동 ***************/
	FHitResult Hit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	// Grounded 상태에서 이동 후 접지: 범프, 경사, 엣지 등에서 뜨는 것 방지
	if (MovementMode == ESkullyMovementMode::Grounded)
	{
		FHitResult StickHit;
		SafeMoveUpdatedComponent(-FVector::UpVector * StickDist, UpdatedComponent->GetComponentQuat(), true, StickHit);
	}
	
	/*************** 8. 충돌 처리 ***************/
	if (Hit.bBlockingHit == true)
	{
		// 8-1. 변수 초기화
		// 속도 2D
		FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
		// 속도 크기
		const float Speed2D = Velocity2D.Size();
		// 충돌 노멀 2D
		FVector HitNormal2D = FVector(Hit.Normal.X, Hit.Normal.Y, 0.0f).GetSafeNormal();
		// 충돌 노멀 유효 플래그
		const bool bHasHitNormal2D = HitNormal2D.IsNearlyZero() == false;
		// 충돌 노멀 방향으로 파고드는 정도(> 0: 파고듦, == 0: 스침, < 0: 떨어짐)
		const float IntoHitNormalVel2D = bHasHitNormal2D == true && Velocity2D.SizeSquared() > KINDA_SMALL_NUMBER ? 
										 FVector::DotProduct(Velocity2D, HitNormal2D) : 0.0f;
		// 정면 충돌 비율(1: 정면 충돌, 0: 스침, < 0: 떨어짐)
		const float IntoRatio = Speed2D > KINDA_SMALL_NUMBER ? IntoHitNormalVel2D / Speed2D : 0.0f;
		// 정면 충돌 플래그
		const bool bHeadOnIntoWall = IntoRatio > WallHeadOnRatioThreshold;
		// 충돌면이 걸을 수 있는지
		const bool bHitSurfaceLooksWalkable = Hit.Normal.Z >= WalkableZ;
		// 벽 판정 플래그
		const bool bTreatAsWall = bHitSurfaceLooksWalkable == false && bHeadOnIntoWall == true && bHasHitNormal2D == true && 
								  Hit.Normal.Z < WallNormalZMax && IntoHitNormalVel2D > MinIntoWallSpeed;
		
		// 8-2. 바닥인 경우(범프, 턱)
		if (bHitSurfaceLooksWalkable == true)
		{
			bHitFloorBumpThisFrame = true;
			
			// 위치 보정
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);
			// 속도 처리
			FVector Projected = FVector::VectorPlaneProject(Velocity2D, Hit.Normal);
			Projected.Z = 0.0f;
			
			// 바닥 충돌로 노멀 튐으로 속도가 급격하게 꺾이는 문제를 방지: 회전량을 제한해 안정화
			if (Speed2D > KINDA_SMALL_NUMBER && Projected.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				const FVector OldDir = Velocity2D / Speed2D; // 기존 방향
				FVector NewDir = Projected.GetSafeNormal(); // 새 방향
				const float MaxTurnRad = FMath::DegreesToRadians(MaxTurnDeg); // 회전 한계값
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
		// 8-3. 벽인 경우
		else if (bTreatAsWall == true)
		{
			bHitWallThisFrame = true;
			
			// 벽 충돌 관련 변수 갱신
			PressWallAlpha = 0.0f;
			bTryingToLeaveWall = false;
			if (bHasInput == true && bHasHitNormal2D == true)
			{
				// 입력 벡터 2D
				const FVector InputDir2D = FVector(InputDir.X, InputDir.Y, 0.0f);
				// 입력 방향이 충돌 노멀 방향으로 파고드는 정도
				const float InputIntoWall = FVector::DotProduct(InputDir2D, HitNormal2D);
				// 벽 쪽으로 파고드는 양만 추출
				PressWallAlpha = FMath::Clamp(InputIntoWall, 0.0f, 1.0f);
				// 벽에서 확실히 이탈하는지(아날로그 스틱 노이즈, 작은 입력값 방지)
				bTryingToLeaveWall = InputIntoWall < -0.1f;
			}
			
			// 위치 보정
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);
			// 속도 처리
			if (bHasHitNormal2D == true && Velocity2D.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				// 이미 떨어지는 중인지
				const bool bSeparatingFromWall = IntoHitNormalVel2D <= 0.0f;
				// 벽에 붙어 있는 상태 플래그
				const bool bLeavingWallNow = bTryingToLeaveWall == true || bSeparatingFromWall == true;

				// 벽에 붙어있는 상태
				if (bLeavingWallNow == false)
				{
					// 벽으로 파고드는 성분 제거/감쇠
					if (IntoHitNormalVel2D > 0.f)
					{
						Velocity2D -= HitNormal2D * IntoHitNormalVel2D * WallNormalKill;
					}
					// 벽으로 파고들수록 접선 성분 감쇠
					if (PressWallAlpha > 0.0f)
					{
						const float TangentialFactor = FMath::Exp(-PressWallAlpha * MaxTangentialKill * 60.0f * DeltaTime);
						Velocity2D *= TangentialFactor;
					}
					// 이탈 상태가 아닐 시 추가 감쇠
					if (bTryingToLeaveWall == false)
					{
						const float Factor = FMath::Clamp(1.0f - WallSlideDamping * DeltaTime, 0.0f, 1.0f);
						Velocity2D *= Factor;
					}

					Velocity.X = Velocity2D.X;
					Velocity.Y = Velocity2D.Y;
				}
				// 벽에 이탈하려는 상태
				else
				{
					if (IntoHitNormalVel2D > 0.f)
					{
						Velocity2D -= HitNormal2D * IntoHitNormalVel2D; // 벽으로 박는 성분 완전 제거
					}

					Velocity.X = Velocity2D.X;
					Velocity.Y = Velocity2D.Y;
				}
			}
			
			// 8-4. Grounded 2-pass: 엣지/코너에서 이동 소실 복구
			// 공중에서 바닥 재투영이 될 수 있음
			if (MovementMode == ESkullyMovementMode::Grounded)
			{
				// 경사 슬라이드로 가속이 들어가 프레임에서는 이동이 2번 먹어서 거리 폭발이 생길 수 있음
				if (bAppliedSlopeSlide == false)
				{
					// 이탈 시 재투영을 해버리면 벽에서 빠지려는 순간 다시 붙는 느낌이 날 수 있음
					if (bTryingToLeaveWall == false)
					{
						const FVector FloorSlide = FVector::VectorPlaneProject(MoveDelta, CachedFloorNormal);
						if (FloorSlide.IsNearlyZero() == false)
						{
							FHitResult FloorHit;
							SafeMoveUpdatedComponent(FloorSlide * 0.5f, UpdatedComponent->GetComponentQuat(), true, FloorHit); // 0.5는 과보정 방지
						}
					}
				}
			}
		}
		// 8-5. 걸을 수 있는 바닥은 아니고 벽으로 확정하기도 애매한 경우
		else
		{
			// 위치 보정
			SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit);
			// 속도 처리
			if (MovementMode == ESkullyMovementMode::Grounded)
			{
				// 충돌 노멀이 아니라 바닥 기준: Hit.Normal은 범프/엣지에서 속도 방향이 계속 찢겨 떨림/끊김
				Velocity2D = FVector::VectorPlaneProject(Velocity2D, GroundFloorNormal);
				// 바닥 범프 취급: 애매한 면에서 경사면 추가 감쇠까지 걸리면 속도가 더 죽고 떨림이 커짐
				bHitFloorBumpThisFrame = true;
			}
			else
			{
				Velocity2D = FVector::VectorPlaneProject(Velocity2D, Hit.Normal);
				bHitFloorBumpThisFrame = false;
			}
			Velocity2D.Z = 0.0f;

			Velocity.X = Velocity2D.X;
			Velocity.Y = Velocity2D.Y;
		}
	}

	/*************** 9. 경사면 접선 감쇠 ***************/
	// 경사면 감쇠 플래그: Grounded 상태 + 입력 존재 + 경사 미끄러짐 상태 + 벽 충돌이 아님 + 바닥 충돌이 아님
	const bool bApplySlopeLateralDamping = MovementMode == ESkullyMovementMode::Grounded && bHasInput == false && 
										   (bSlopeSlideState == true || bComputedSlopeSlide == true) && 
										   bHitWallThisFrame == false && bHitFloorBumpThisFrame == false;
	if (bApplySlopeLateralDamping == true)
	{
		FVector Downhill3D = GetDownhillDir(GroundFloorNormal);
		Downhill3D = FVector(Downhill3D.X, Downhill3D.Y, 0.0f).GetSafeNormal();
		
		if (Downhill3D.IsNearlyZero() == false)
		{
			// 속도
			FVector Velocity2D(Velocity.X, Velocity.Y, 0.0f);
			// 속도를 경사면 아래 방향으로 투영
			const float Along = FVector::DotProduct(Velocity2D, Downhill3D);
			// 속도에서 다운힐 성분 추출
			const FVector AlongV = Downhill3D * Along;
			// 속도에서 경사면의 접선 성분 추출
			FVector LateralV = Velocity2D - AlongV;

			// 접선 성분 감쇠
			const float Factor = FMath::Clamp(1.0f - LateralDamping * DeltaTime, 0.0f, 1.0f);
			LateralV *= Factor;
			
			Velocity2D = AlongV + LateralV;
			Velocity.X = Velocity2D.X;
			Velocity.Y = Velocity2D.Y;
		}
	}

	/*************** 10. 이번 프레임 실제 이동량 기반 동기화 및 비주얼 롤링 변수 ***************/
	const FVector End = UpdatedComponent->GetComponentLocation();
	LastActualDelta = End - Start;

	/*************** 11. 폭발 방지 ***************/
	// 속도 동기화를 계산해도 되는 상황인지
	const bool bCanSync = DeltaTime > KINDA_SMALL_NUMBER && MovementMode == ESkullyMovementMode::Grounded;
	// 벽 때문에 이동이 제약된 프레임인지: 벽에 충돌 + 벽에 파고듦 + 벽을 이탈중이지 않을 때 
	const bool bConstrainedByWall = bHitWallThisFrame == true && PressWallAlpha > MinPressWallAlphaForConstrainedFrame && bTryingToLeaveWall == false;
	//너무 가파른 경사에서 입력을 넣고 있어서 이동이 제약된 프레임: 가파른 경사 + 입력 중
	const bool bConstrainedBySteepInput = bTooSteepNow == true && bHasInput == true; 
	// 폭발 방지 유효 플래그
	const bool bConstrainedFrame = bCanSync && (bConstrainedByWall || bConstrainedBySteepInput);
	// 제약이 강하게 걸린 프레임에서는 속도를 실제 이동량쪽으로 끌어와서 에너지/상태를 현실에 맞춤
	// 제약 프레임: 투영, SlideAlongSuface, StickDist, 벽 처리, 경사 업힐 제거 등이 많아 Velocity는 갱신됐는데 실제 이동은 거의 못함
	if (bConstrainedFrame == true)
	{
		// 이번 프레임 실제 이동량 변화량을 속도로 환산: 실제로 이동시킨 속도
		FVector ActualVel2D = LastActualDelta / DeltaTime;
		ActualVel2D.Z = 0.0f;
		// 실제 이동 속도가 유효한 프레임에서만 동기화: 턱, 미세 충돌, 메시 경계같은 곳에서는 강제 동기화 시 속도가 0에 가까워져 멈춤
		if (ActualVel2D.SizeSquared() > FMath::Square(MinActualSpeedForSync))
		{
			const float SyncAlpha = ConstrainedVelocitySyncAlpha;
			const FVector CurVel2D(Velocity.X, Velocity.Y, 0.0f);
			const FVector NewVel2D = FMath::Lerp(CurVel2D, ActualVel2D, SyncAlpha);
			
			Velocity.X = NewVel2D.X;
			Velocity.Y = NewVel2D.Y;
		}
	}
}

FVector USkullyMovementComponent::ConsumeMovementInput()
{
	FVector Input = ConsumeInputVector(); // 입력으로 쌓인 벡터(PendingInputVector)을 소비(가져오고 초기화)

	if (Input.IsNearlyZero() == false)
	{
		Input = Input.GetClampedToMaxSize(1.0f); // 입력 벡터의 크기를 1로 제한: 대각선 과속 방지
	}

	return Input;
}

FVector USkullyMovementComponent::GetDownhillDir(const FVector& FloorN)
{
	const FVector GravityVec = -FVector::UpVector;
	FVector AlongPlane = FVector::VectorPlaneProject(GravityVec, FloorN);
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
				bSlopeSlideState = false; // 미끄러짐 상태 끔
				
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
			UnwalkableSlopeNormal = Hit.ImpactNormal.GetSafeNormal(); // ApplyUnwalkableSlide 함수에서 사용할 바닥 노멀
			LastFloorNormal = CachedFloorNormal;
			CachedFloorNormal = UnwalkableSlopeNormal;
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
		bSlopeSlideState = false;
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
	bSlopeSlideState = false;
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
