// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SkullyMovementComponent.generated.h"

UENUM(BlueprintType)
enum class ESkullyMovementMode : uint8
{
	Grounded UMETA(DisplayName = "Grounded"),
	Falling UMETA(DisplayName = "Falling")
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
	
	// 점프 요청
	void RequestJump();
	// 점프 종료
	void RequestJumpRelease();
	
protected:
	// 중력 적용
	void ApplyGravity(float DeltaTime);
	// 점프 로직
	void TryConsumeJump();
	// 점프 조건
	bool CanJump() const;
	// 저항 적용
	void ApplyFriction(float DeltaTime, float GroundedFriction);
	// 경사면에서 정지 시 미끄러짐(굴러떨어짐) 적용
	// 반환값: 이번 프레임 슬라이드 가속을 적용했는지 (슬라이딩 마찰로 전환할지 판단용)
	bool ApplySlopeSlide(float DeltaTime);
	// 이동 처리
	void Move(float DeltaTime);
	// 굴러가는 효과
	void ApplyVisualRoll(const FVector& ActualDelta);
	// 지면 판정
	void CheckGround(float DeltaTime);
	// 이동에 관련된 상태값 갱신
	void UpdateMotionState();
	// 바닥 감지
	bool SweepGround(FHitResult& OutHit);
	// 바닥 위치 보정
	void SnapToGround(const FHitResult& Hit);
	
	// 입력 벡터 소비 및 정규화
	FVector ConsumeMovementInput();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	USceneComponent* VisualComponent = nullptr;
	
protected:	
	// 중력값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float Gravity = 2000.0f;
	
	// 상승 중
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float JumpGravityScale = 1.0f;
	
	// 하강 중
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float FallGravityScale = 2.0f;
	
	// 최대 속력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MaxSpeed = 3500.0f;
	
	// 현재 속력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float CurrentSpeed2D = 0.0f;
	
	// 현재 이동 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	FVector CurrentMoveDir2D = FVector::ZeroVector;
	
	// 가속값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float Acceleration = 8000.0f;
	
	// 마찰력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Friction")
	float GroundFriction = 1500.0f;
	
	// 경사면 미끄러짐(굴러떨어짐) 중 적용할 마찰력(운동 마찰 느낌)
	// 너무 크면 슬라이드가 죽고, 너무 작으면 얼음처럼 내려감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Friction")
	float SlidingFriction = 50.0f;
	
	// 공기 저항력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air|Friction")
	float AirFriction = 5000.0f;
	
	// 점프 속도
	UPROPERTY(EditAnywhere, Category="Jump")
	float JumpSpeed = 2000.0f;

	// 점프 중 바닥 붙잡임 무시 시간
	UPROPERTY(EditAnywhere, Category="Jump")
	float JumpIgnoreGroundTime = 0.08f;
	
	// 정지 마찰을 각도대신 가속도 임계치로 모델링하기 위한 값
	// 중력의 경사 성분(VectorPlaneProject(Gravity, FloorN)이 이 값보다 작으면 완전 정지 유지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float StaticFrictionAccel = 80.0f;
	
	// 슬라이드가 막 시작될 대(속도 거의 0) 한 프레임 멈칫하는 현상 방지용 최소 시작 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float MinSlopeSlideStartSpeed = 120.0f;
	
	// 엣지/경계에서 노멀이 튀어 슬라이드가 멈추는 것을 완화하기 위한 downhill 샘플 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float DownhillSampleDistance = 10.0f;
	
	// 오를 수 있는 경사면 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float MaxSlopeAngle = 45.0f;
	
	// 정지 상태에서 버틸 수 있는 경사각(정지 마찰)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float StopSlopeAngle = 10.0f;
	
	// 경사면 미끄러짐 가속 스케일(1.0이면 중력의 경사 성분 그대로)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float SlopeSlideScale = 3.0f;
	
	// 경사면에서 내려가는 최대 속도 제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float MaxSlopeSlideSpeed = 6000.0f;
	
	// 슬라이딩 중 속도 댐핑 계수(속도 비례 감쇠)
	// 값이 클수록 빨리 감속되고 종단속도가 낮아짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float SlideDamping = 0.2f;
	
	// 지면 감지 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float GroundCheckDistance = 5.0f;
	
	// CachedFloorNormal.Z가 얼마나 위를 향하느냐(1에 가까울수록 평지, 작아질수록 가파른 경사)
	// 줄이면? 불안정 판정이 덜 자주 발생, 경사면 이동이 더 정상적으로 되나 특이점(엣지, 꼭지점 등) 탈출이 약해짐
	// 늘리면? 조금만 기울어도 불안정 판정, 투영 이동을 거의 안 하고 fallback이 늘어나 경사 오르기/내리기 느낌이 망가짐 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float UnstableFloorZThreshold = 0.97f;
	
	// 노멀 변화량 지표(1: 완전 동일, 0.99: 거의 동일, 0.95: 꽤 달라짐, 0.9이하: 확실히 급변)
	// 줄이면? 엣지 판정이 덜 민감해짐, 경계면에서 멈춤이 다시 나올 확률이 높아지나 흔들림이 줄어듬
	// 늘리면? 아주 조금만 노멀 변해도 엣지 판정, 경계 탈출이 강해지나 정상 경사에서도 불필요하게 fallback이 증가해 미세한 떨림 및 감각이 미끄덩해질 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float FloorNormalDotEdgeThreshold = 0.95f;
	
	// 투영 후 최소 이동량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float MinProjectedMoveCm = 1.0f;
	
	// 바닥이라고 인정할 수 있는 최대 거리(cm)
	// 스윕은 맞았는데 바닥이 너무 멀리 있다면 곧중에 떠 있는 걸로 처리(Falling)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float MaxGroundDistance = 8.0f;
	
	// GraceZ의 오프셋
	// 값을 키우면? 경계면/꼭짓점에서 Grounded가 안정적이고 끊김/떨림 감소, Walkable 아닌 경사에서도 붙어있으려는 성향 증가
	// 값을 줄이면? 판정이 정확해지고 가파른 곳에서 떨어지는 것이 자연스러워짐, 경계/꼭짓점에서 Grounded 깜빡임이 다시 생김
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float GroundGraceZOffset = 0.05f;
	
	// CachedFloorNormal에서 Hit.ImpactNormal로의 보간 속도
	// 클수록 더 빨리 목표 노멀에 가까워짐, 경사면 적용이 즉각적이고 반응이 시원해짐, 노멀이 확 튀면서 끊김/멈춤/떨림 발생
	// 작을수록 더 천천히 따라감, 노멀이 튀는 현상 완화와 이동이 매끈해짐, 경사면 변화에 대한 반응이 느림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float FloorNormalInterpSpeed = 12.0f;
	
	// 어느 각도부터 경사로 보고 스냅을 막을지
	// 줄이면? 각도가 낮아져 더 안정적인 착지
	// 늘리면? 각도가 높아져 경사면에서 과도하게 달라붙는 느낌이 생길 수도 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float FlatGroundZThreshold = 0.997f;
	
	// 아래로 라인트레이스를 쏘는 거리
	// MaxGroundDistance의 값에 따라 더 큰 적당한 값으로 설정
	UPROPERTY(EditAnywhere, Category="Movement|Ground")
	float GroundLineTraceDistance = 12.0f;
	
	// 벽에 부딪히고 차당 감쇠 강도(클수록 빨리 줄어듦)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wall")
	float WallSlideDamping = 6.0f;
	
	// 벽에 부딪히고 노멀 성분 제거(1이면 노멀 성분 완전 제거, 0이면 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wall")
	float WallNormalKill = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	bool bRollVisualOnMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	float RollVisualScale = 1.0f; // 과장/완화
	
private:
	// 움직임 상태값
	ESkullyMovementMode MovementMode = ESkullyMovementMode::Falling;
	
	// 가속도
	FVector Velocity = FVector::ZeroVector;
	
	// 이번 프레임에 ApplySlopeSlide가 Velocity에 실제로 가속을 적용했는지
	bool bSlopeSlideAppliedThisFrame = false;
	
	// 이전에 캐싱된 바닥 노멀
	FVector LastFloorNormal = FVector::UpVector;
	
	// 바닥 노멀 캐싱 변수
	FVector CachedFloorNormal = FVector::UpVector;
	
	// 현재 스윕 히트
	FHitResult CurrentFloorHit;
	
	// 슬라이딩 플래그
	bool bIsSlopeSliding = false;
	
	FVector PendingSlopeSlideAccel2D = FVector::ZeroVector;
	bool bSlopeSlideThisFrame = false;
	
	// 스페이스를 누르고 있는가
	bool bJumpHeld = false;
	// 점프 예약 플래그
	bool bWantsToJump = false;
	// 점프 버퍼
	float JumpBufferTime = 0.12f;
	float JumpBufferRemaining = 0.0f;
	// 바닥 붙잡임 무시 남은 시간
	float JumpIgnoreGroundRemaining = 0.0f;
	
	FVector LastActualDelta = FVector::UpVector;
	
	bool bOnUnwalkableSlope = false;
	FVector UnwalkableNormal = FVector::UpVector;
	
};