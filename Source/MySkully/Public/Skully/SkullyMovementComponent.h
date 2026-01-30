#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SkullyMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMovementChanged, float DeltaTime, float Speed, FVector Dir);

// 이동 상태
UENUM(BlueprintType)
enum class ESkullyMovementMode : uint8
{
	Grounded UMETA(DisplayName = "Grounded"),
	Falling UMETA(DisplayName = "Falling")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYSKULLY_API USkullyMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:
	USkullyMovementComponent();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FOnMovementChanged OnMovementChanged;
	
	FORCEINLINE ESkullyMovementMode GetMovementMode() const { return MovementMode; }
	FORCEINLINE FVector GetLastActualDelta() const { return LastActualDelta; }
	
	/********************점프********************/
	void RequestJump(); // 점프 요청
	void RequestJumpRelease(); // 점프 종료 요청
	
protected:
	/******************바닥 감지******************/
	void CheckGround(float DeltaTime); // 지면 판정
	bool SweepGround(FHitResult& OutHit) const; // 지면으로 스윕
	void SnapToGround(const FHitResult& Hit) const; // 지면 스냅
	bool TryConfirmGroundByLineTrace(float WalkableZ); // 보조 바닥 판정
	
	/********************중력********************/
	void ApplyGravity(float DeltaTime); // 중력 적용
	
	/********************이동********************/
	void Move(float DeltaTime); // 이동 처리
	FVector ConsumeMovementInput(); // 입력 벡터 소비
	FVector GetDownhillDir(const FVector& FloorN); // 아래(중력) 방향을 바닥 평면에 투영(downhill)
	
	/******************미끄러짐*******************/
	void ApplyUnwalkableSlide(float DeltaTime); // 걸을 수 없는 경사면(가파른 경사면)에서 미끄러짐(굴러떨어짐)->Falling 상태지만 충돌 때문에 잘 안미끄러지는 상황 탈출
	bool ApplySlopeSlide(float DeltaTime); // 걸을 수 있는 경사면(완만한 경사면)에서 입력이 없을 때 미끄러짐(굴러떨어짐)->슬라이딩 가속 변수 설정
	void ApplyFriction(float DeltaTime); // 마찰 적용
	
	/*****************구르기 연출*****************/
	void ApplyVisualRoll(const FVector& ActualDelta) const; // 입력 방향에 따라 메시 굴리기
	
	/********************점프********************/
	void TryConsumeJump(); // 점프 메인 로직
	bool CanJump() const; // 점프 가능 판정
	bool TryStartJumpFromBuffer(); // 점프 버퍼 소비 판정
	void UpdateJumpBufferTimer(float DeltaTime); // 점프 버퍼 대기 시간 갱신
	
	/******************출력 변수******************/
	void UpdateMovementState(float DeltaTime); // 출력 변수 갱신
	
public:
	/********************이동********************/
	// 최대 속력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MaxSpeed = 4000.0f;
	
	/*****************구르기 연출*****************/
	// 회전시킬 메시 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roll")
	USceneComponent* VisualComponent = nullptr;
	
protected:	
	/******************바닥 감지******************/
	// 지면 스윕 감지 거리
	// 보통 지면 판정 거리(MaxGroundDistance)의 3배
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float GroundCheckDistance = 12.0f;
	// GraceZ 오프셋(0.05=약 4도)
	// GraceZ: 노멀 튐(경계, 경사, 범프 등)을 위한 관용값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|GraceZ")
	float GroundGraceZOffset = 0.05f;
	// 지면 판정 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float MaxGroundDistance = 4.0f;
	// 경사 판정값
	// 1: 평지 ~ 0: 벽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float FlatGroundZThreshold = 0.997f;
	// 걸을 수 있는 경사면에서의 이전 바닥 노멀과 새로운 바닥 노멀의 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Stability")
	float FloorNormalInterpSpeed = 12.0f;
	// 미끄러질 경사면 기준값(0~1)
	// 1: 평지 ~ 0: 벽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Stability")
	float MinSlopeZForSlide = 0.1f;
	// 보조 바닥 판정 지면 라인 트레이스 감지 거리
	// 지면 스윕 감지 거리(GroundCheckDistance)보다 조금 길게
	UPROPERTY(EditAnywhere, Category="Ground|Stability")
	float GroundLineTraceDistance = 15.0f;
	
	/********************중력********************/
	// 중력값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float Gravity = 3000.0f;
	// 점프 중력 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float JumpGravityScale = 1.0f;
	// 낙하 중력 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float FallGravityScale = 2.0f;
	// 경사면 접지값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float StickZ = 12.0f;
	
	/********************이동********************/
	// 가속값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float Acceleration = 5000.0f;
	// 목표 속도 생성 시 접선 성분 기본 감쇠값(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Target")
	float UphillPushStartThreshold = 0.6f;
	// 목표 속도 생성 시 접선 성분 최대 감쇠값(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Target")
	float UphillPushFullThreshold  = 1.0f;
	// 목표 속도 생성 시 유효 입력(바닥 투영)으로 인정할 최소값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Target")
	float MinInputMagnitudeForTargetVelocity2D = 0.2f;
	// 바닥 노멀 Z가 이 값보다 작으면 불안정한 바닥(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float UnstableFloorZThreshold = 0.97f;
	// 바닥 노멀 변화량이 이 값보다 작으면 불안정한 바닥(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float UnstableFloorNormalContinuityThreshold = 0.95f;
	// 안정한 바닥 이동일 때 유효 투영으로 인정할 최소값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float MinProjectedMoveCm = 1.0f;	
	// 이동 후 접지값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	float StickDist = 2.0f;
	// 정면 충돌 비율이 이 값보다 크면 정면 충돌로 인정할 임계값(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float WallHeadOnRatioThreshold = 0.65f;
	// 충돌 노멀이 이 값보다 작으면 벽으로 인정할 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float WallNormalZMax = 0.2f;
	// 충돌 노멀 방향으로 파고드는 정도가 이 값보다 크면 벽 충돌로 인정할 임계값 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float MinIntoWallSpeed = 200.0f;
	// 바닥 충돌 시 노멀 튐으로 속도 껶임을 방지할 회전 한계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Ground")
	float MaxTurnDeg = 25.0f;
	// 벽 충돌 시 파고드는 성분 감쇠값(1이면 성분 완전 제거, 0이면 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float WallNormalKill = 1.0f;
	// 벽 충돌 시 접선 성분 감쇠값(1이면 성분 완전 제거, 0이면 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float MaxTangentialKill = 0.45f;
	// 벽 충돌 시 이탈 상태가 아닐 시 추가 감쇠값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Hit|Wall")
	float WallSlideDamping = 12.0f;
	// 경사면 접선 감쇠 성분 감쇠값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Slope")
	float LateralDamping = 10.0f;
	// 폭발 방지 시 벽에 파고드는 정도가 이 값보다 크면 동기화할 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Wall")
	float MinPressWallAlphaForConstrainedFrame = 0.05f;
	// 동기화 시 미세하게 움직인 프레임 방지값 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Wall")
	float MinActualSpeedForSync = 5.0f;
	// 동기화 시 현재 속도를 얼만큼 보간할지 보간값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Wall")
	float ConstrainedVelocitySyncAlpha = 0.35;
	
	/******************미끄러짐*******************/
	// 마찰력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Friction")
	float GroundFriction = 1500.0f;
	// 공기 저항력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air|Friction")
	float AirFriction = 5000.0f;
	// 오를 수 있는 경사면 각도
	// MaxSlopeAngle=Cos(MaxSlopeAngle): 0도=1, 30도=0.866, 45도=0.707, 60도=0.5, 90도=0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope")
	float MaxSlopeAngle = 45.0f;
	// 미끄러짐 속력(최대 속력의 약 2배)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed|Slope")
	float MaxSlopeSlideSpeed = 7500.0f;
	// 걸을 수 없는 경사면(가파른 경사면)의 가속 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float UnwalkableSlopeSlideScale = 1.8f;
	// 걸을 수 없는 경사면(가파른 경사면)에서 보장되는 최소 가속 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float UnwalkableSlopeMinAcceleration = 2200.f;
	// 다운힐 샘플링 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Sample")
	float DownhillSampleDistance = 10.0f;
	// 다운힐 샘플링 추가 라인트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Sample")
	float DownhillSampleLinTraceExtraDistance = 50.0f;
	// 완만한 경사에서도 미끄러지게 만드는 감성 커브값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float SlopeSlideCurveExponent = 0.25;
	// 슬라이딩 최소 가속 크기 최소값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float SlopeSlideMinAccelLow = 1600.0f;
	// 슬라이딩 최소 가속 크기 최대값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float SlopeSlideMinAccelHigh = 4200.0f;
	// 슬라이딩 유지 가속도 임계값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slope|Slide")
	float SlopeSlideStopAccel = 600.0f;
	// 정지 마찰 계수: 경사의 기울기가 이 값보다 크면 슬라이딩
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slope|Slide")
	float StaticFrictionMu = 0.25;
	
	/*****************구르기 연출*****************/
	// 구르기 연출 On/Off 플래그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roll")
	bool bRollVisualOnMove = true;
	// 구르기 연출용 배율
	// 1.0: 물리적으로 정확, 0.8: 좀 덜 구르는 느낌, 1.2: 좀 더 빠르게 구르는 느낌  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roll")
	float RollVisualScale = 1.0f;
	
	/********************점프********************/
	// 점프 속도
	UPROPERTY(EditAnywhere, Category="Jump")
	float JumpSpeed = 2500.0f;
	// 가변 점프 스케일(0~1)
	// ex) 0.7=상송 속도를 30% 감소
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jump|Input")
	float JumpReleaseVelocityScale = 0.5f;
	// 점프 바닥 감지 무시 시간
	// 점프 시작 혹은 다음 프레임에서는 지면과 가깝기 때문에 버그 발생이 높음 
	UPROPERTY(EditAnywhere, Category="Jump|Input")
	float JumpIgnoreGroundTime = 0.08f;
	// 점프 버퍼 유지 시간(초)
	// 이 시간 이내로 착지하면 점프 선입력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jump|Input")
	float JumpBufferTime = 0.12f;
	// 코요테 타임 유지 시간(초)
	// 착지 직후에도 점프를 허용하는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jump|Input")
	float CoyoteTime = 0.08f;
	
public:
	/******************출력 변수******************/
	// 현재 속력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float CurrentSpeed2D = 0.0f;
	// 현재 이동 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	FVector CurrentDir2D = FVector::ZeroVector;
	// 현재 히트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground")
	FHitResult CurrentFloorHit;
	
private:
	ESkullyMovementMode MovementMode = ESkullyMovementMode::Falling; // 움직임 상태값

	/******************캐싱*******************/
	FVector LastFloorNormal = FVector::UpVector; // 이전 프레임 바닥 노멀
	FVector CachedFloorNormal = FVector::UpVector; // 바닥 노멀 캐싱
	
	/********************이동********************/
	FVector LastActualDelta = FVector::UpVector; // Move함수 실제 이동량 기반 동기화용 변수
	
	/******************미끄러짐*******************/
	bool bOnUnwalkableSlope = false; // 걸을 수 없는 경사면(가파른 경사면) 플래그(=슬라이딩 플래그)->CheckGround에서 설정
	FVector UnwalkableSlopeNormal = FVector::UpVector; // 걸을 수 없는 경사면(가파른 경사면)의 바닥 노멀->CheckGround에서 설정
	bool bSlopeSlideState = false; // 슬라이딩 상태 플래그
	bool bComputedSlopeSlide = false; // 이번 프레임에서 슬라이딩 가속 계산 플래그->ApplySlopeSlide함수 맨 끝에서 설정
	bool bAppliedSlopeSlide = false; // 이번 프레임에 슬라이드 가속 사용 플래그->Move함수에서 실제 가속을 더 했는지
	FVector PendingSlopeSlideAccel2D = FVector::ZeroVector; // 걸을 수 있는 경사면(완만한 경사면)에서의 가속값->Move에서 처리
	float CachedSlopeAmount = 0.0f; // 이번 프레임 경사량 캐시->마찰/감쇠에 사용
	
	/********************점프********************/
	bool bJumpHeld = false; // 점프가 입력 중인지 체크
	bool bWantsToJump = false; // 점프 예약 플래그
	float JumpIgnoreGroundRemaining = 0.0f; // 점프 바닥 감지 무시 남은 시간
	float JumpBufferRemaining = 0.0f; // 점프 버퍼 남은 시간
	float CoyoteRemaining = 0.0f; // 코요테 타임 남은 시간
};
