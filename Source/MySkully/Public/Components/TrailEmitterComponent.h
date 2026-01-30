#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrailEmitterComponent.generated.h"


class USkullyMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYSKULLY_API UTrailEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTrailEmitterComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	// MovementComponent 콜백
	void OnMoveChanged(float DeltaTime, float Speed, FVector Dir);
	// 현재 상태에서 트레일 찍어도 되는지
	bool CanEmit() const;
	// 스탬프 한 번 요청
	void EmitStamp(float Strength);
	// 0~1 랩핑 시간(페이드용)
	float GetTimeWrapped() const;
	
private:
	UPROPERTY()
	TObjectPtr<USkullyMovementComponent> MoveComp = nullptr;
	
	// 몇 cm마다 스탬프 1개(스컬리 굴러간 자국은 보통 20~50cm 선이 예쁨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float StampSpacingCm = 30.0f;
	// 스탬프 반지름(cm): 스탬프 머티리얼의 TrailRadius(0~1)와는 다른 월드 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float StampRadiusCm = 45.0f;
	// 속도가 이 값보다 작으면 스탬프 안 찍기(쓸데없는 도장 방지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float MinSpeedToEmitCmPerSec = 50.0f;
	// Strength를 속도 기반으로 계산할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	bool bStrengthFromSpeed = true;
	// 이 속도에서 Strength=1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float SpeedForMaxStrengthCmPerSec = 1200.0f;
	// 속도 기반이 아니면 고정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float ConstantStrength = 1.0f;
	// 너무 가파른 바닥은 제외(노멀 UpDot 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float MinUpDotToEmit = 0.25f;
	// 시간 랩핑 주기(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emit", meta = (AllowPrivateAccess = true))
	float WrapPeriodSec = 8.0f;
	
	// 런타임 상태
	FVector LastEmitLocation = FVector::ZeroVector;
	bool bHasLastEmit = false;
	
	float AccumulatedDistanceCm = 0.0f;
	
};
