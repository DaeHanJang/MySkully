#include "Components/TrailEmitterComponent.h"

#include "Skully/SkullyMovementComponent.h"
#include "Trail/TrailManagerSubsystem.h"
#include "Engine/World.h"

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
	
	AccumulatedDistanceCm = 0.0f;
	
	// 델리게이트 핸들 저장
	MovementChangedHandle = MoveComp->OnMovementChanged.AddUObject(this, &UTrailEmitterComponent::OnMoveChanged);
}

void UTrailEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MoveComp != nullptr && MovementChangedHandle.IsValid() == true)
	{
		// 멀티캐스트에서 제거(안 하면 EndPlay 이후 호출 위험)
		MoveComp->OnMovementChanged.RemoveAll(this);
		MovementChangedHandle.Reset();
	}
	
	Super::EndPlay(EndPlayReason);
}

void UTrailEmitterComponent::OnMoveChanged(float DeltaTime, float Speed, FVector Dir)
{
	if (MoveComp == nullptr)
	{
		return;
	}
	
	// 1. 상태가 트레일을 찍을 수 없으면 누적도 하지 않음
	if (CanEmit() == false)
	{
		AccumulatedDistanceCm = 0.0f;
		return;
	}
	
	// 2. 속도가 너무 낮으면 누적하지 않음
	if (Speed < MinSpeedToEmitCmPerSec)
	{
		return;
	}
	
	// 3. 이동거리 누적: 실제 이동량 기반
	const FVector ActualDelta = MoveComp->GetLastActualDelta();
	const float DeltaDist = FVector(MoveComp->GetLastActualDelta().X, MoveComp->GetLastActualDelta().Y, 0.0f).Size();	
	if (DeltaDist <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	AccumulatedDistanceCm += DeltaDist;
	
	// 4. 고속 이동에서 스탬프가 끊기지 않게 while로 여러 개 찍기
	const float Strength = ComputeStrength(Speed);
	
	while (AccumulatedDistanceCm >= StampSpacingCm)
	{
		EmitStamp(Strength);
		AccumulatedDistanceCm -= StampSpacingCm;
		
		// 무한루프 방지(혹시 StampSpacing이 0으로 들어오면)
		if (StampSpacingCm <= KINDA_SMALL_NUMBER)
		{
			AccumulatedDistanceCm = 0.0f;
			return;
		}
	}
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

float UTrailEmitterComponent::ComputeStrength(float Speed) const
{
	if (bStrengthFromSpeed == false)
	{
		return FMath::Clamp(ConstantStrength, 0.0f, 1.0f);
	}
	
	if (SpeedForMaxStrengthCmPerSec <= 1.0f)
	{
		return 1.0f;
	}
	
	return FMath::Clamp(Speed / SpeedForMaxStrengthCmPerSec, 0.0f, 1.0f);
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
	
	// 바닥 접점/노멀 사용
	const FHitResult& FloorHit = MoveComp->CurrentFloorHit;
	
	FTrailStampRequest Req;
	Req.Location = FloorHit.ImpactPoint;
	Req.Normal = FloorHit.ImpactNormal;
	Req.Radius = StampRadiusCm;
	Req.Strength = Strength;
	
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
