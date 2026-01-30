#include "Components/TrailEmitterComponent.h"

#include "Skully/SkullyMovementComponent.h"
#include "Trail/TrailManagerSubsystem.h"

UTrailEmitterComponent::UTrailEmitterComponent()
{
	// 콜백 기반이라 Tick 필요 없음
	PrimaryComponentTick.bCanEverTick = false;
}

void UTrailEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}
	
	// 스컬리 전용 MovementComponent 찾기
	MoveComp = Owner->FindComponentByClass<USkullyMovementComponent>();
	if (MoveComp == nullptr)
	{
		return;
	}
	
	// 초기화
	LastEmitLocation = Owner->GetActorLocation();
	bHasLastEmit = true;
	AccumulatedDistanceCm = 0.0f;
	
	// MovementChanged 이벤트 구독
	MoveComp->OnMovementChanged.AddUObject(this, &UTrailEmitterComponent::OnMoveChanged);
}

void UTrailEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MoveComp != nullptr)
	{
		// 멀티캐스트에서 제거(안 하면 EndPlay 이후 호출 위험)
		MoveComp->OnMovementChanged.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UTrailEmitterComponent::OnMoveChanged(float DeltaTime, float Speed, FVector Dir)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || MoveComp == nullptr)
	{
		return;
	}
	// 속도 너무 낮으면 누적도 하지 않는 쪽이 보통 덜 지저분 함
	if (Speed < MinSpeedToEmitCmPerSec)
	{
		return;
	}
	
	if (bHasLastEmit == false)
	{
		LastEmitLocation = Owner->GetActorLocation();
		bHasLastEmit = true;
		return;
	}
	
	// 이동거리 누적
	const FVector CurLoc = Owner->GetActorLocation();
	const float DeltaDist = FVector(MoveComp->GetLastActualDelta().X, MoveComp->GetLastActualDelta().Y, 0.0f).Size();
	
	AccumulatedDistanceCm += DeltaDist;
	
	if (AccumulatedDistanceCm < StampSpacingCm)
	{
		return;
	}
	
	if (CanEmit() == false)
	{
		return;
	}
	
	// Strength 계산
	float Strength = ConstantStrength;
	if (bStrengthFromSpeed == true)
	{
		Strength = SpeedForMaxStrengthCmPerSec > 1.0f ? FMath::Clamp(Speed / SpeedForMaxStrengthCmPerSec, 0.0f, 1.0f) : 1.0f;
	}
	
	EmitStamp(Strength);;
	
	// 리셋
	AccumulatedDistanceCm = 0.0f;
	LastEmitLocation = CurLoc;
}

bool UTrailEmitterComponent::CanEmit() const
{
	// MovementComponent가 없을 경우
	if (MoveComp == nullptr)
	{
		return false;
	}
	// 공중이면 끄기
	if (MoveComp->GetMovementMode() != ESkullyMovementMode::Grounded)
	{
		return false;
	}
	// 바닥 히트가 유효해야 함
	if (MoveComp->CurrentFloorHit.bBlockingHit == false)
	{
		return false;
	}
	
	// 너무 가파른 곳 제외
	const FVector N = MoveComp->CurrentFloorHit.ImpactNormal.GetSafeNormal();
	const float UpDot = FVector::DotProduct(N, FVector::UpVector);
	if (UpDot < MinUpDotToEmit)
	{
		return false;
	}
	
	return true;
}

void UTrailEmitterComponent::EmitStamp(float Strength)
{
	UWorld* World = GetWorld();
	if (World == nullptr || MoveComp == nullptr)
	{
		return;
	}
	
	UTrailManagerSubsystem* TrailMgr = World->GetSubsystem<UTrailManagerSubsystem>();
	if (TrailMgr == nullptr)
	{
		return;
	}
	
	const FHitResult& FloorHit = MoveComp->CurrentFloorHit;
	
	FTrailStampRequest Req;
	Req.Location = FloorHit.ImpactPoint;
	Req.Normal = FloorHit.ImpactNormal;
	Req.Radius = StampRadiusCm;
	Req.Strength = Strength;
	Req.TimeWrapped = GetTimeWrapped();
	
	TrailMgr->RequestStamp(Req);
}

float UTrailEmitterComponent::GetTimeWrapped() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr || WrapPeriodSec <= 0.0f)
	{
		return 0.0f;
	}
	
	return FMath::Frac(static_cast<float>(World->GetTimeSeconds()) / WrapPeriodSec);
}
