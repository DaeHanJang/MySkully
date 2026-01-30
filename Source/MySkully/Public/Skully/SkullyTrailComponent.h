#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkullyTrailComponent.generated.h"

class ASkullyTrailStampActor;
class USkullyMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYSKULLY_API USkullyTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USkullyTrailComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// MovementComponent 델리게이트 콜백
	void HandleMovementChanged(float DeltaTime, float Speed2D, FVector Dir2D);
	// 지정 위치에서 스탬프 찍기 (연속 트레일용)
	bool TryStampAt(const FVector& SampleWorldPos, const FVector& MoveDir2D);
	// 바닥 트레이스(지정 위치 기준)
	bool TraceGroundAt(const FVector& SampleWorldPos, FHitResult& OutHit) const;
	// 스탬프 한 개 배치
	void PlaceStampInstance(const FTransform& WorldXform, float SpawnTimeSeconds, float Strength);
	
public:
	// 기능 On/Off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	bool bEnableTrail = true;
	// 타겟 RVT (스탬프가 여기에 써짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	URuntimeVirtualTexture* TargetRVT = nullptr;
	// 스탬프 메쉬(보통 Plane)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	UStaticMesh* StampMesh = nullptr;
	// 스탬프 머티리얼(RVT Output 포함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	UMaterialInterface* StampMaterial = nullptr;
	// 샘플링 간격(cm): 움직인 거리 누적이 이 값 넘어가면 스탬프
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float SampleSpacingCm = 6.0f;
	// 트레일 폭(cm): Plane X/Y 스케일로 반영(메쉬 축에 따라 달라질 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float TrailWidthCm = 160.0f;
	// 스탬프 길이(cm): 진행방향으로 늘리고 싶으면 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float StampLengthCm = 160.0f;
	// 지면에서 띄우는 오프셋(cm): z-fighting 방지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float SurfaceOffsetCm = 0.7f;
	// 너무 느릴 땐 안 찍기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float MinSpeedToStamp = 5.0f;
	// 방향 정렬(진행 방향으로 스탬프 길이를 맞추고 싶으면 true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	bool bAlignToMovementDir = true;
	// 트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float TraceUpCm = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float TraceDownCm = 200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	// 풀링 최대 인스턴스 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	int32 MaxInstances = 2048;
	// 머티리얼에서 PerInstanceCustomData[0] = SpawnTimeSeconds 로 Face 계산하려면 true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	bool bWriteSpawnTimeToCustomData = true;
	// Fade 시간
	UPROPERTY(EditAnywhere, Category="Fade")
	float FadeDurationSeconds = 4.0f;
	// 스탬프 객체 클래스
	UPROPERTY(EditAnywhere, Category="Trail")
	TSubclassOf<ASkullyTrailStampActor> StampActorClass;
	// 스탬프 객체
	UPROPERTY(Transient)
	TObjectPtr<ASkullyTrailStampActor> StampActor = nullptr;
	
private:
	UPROPERTY()
	USkullyMovementComponent* MoveComp = nullptr;
		
	FVector LastSamplePos = FVector::ZeroVector;
	bool bHasLastSample = false;
	// 나머지 거리 누적용(보간 스탬프에 중요)
	float DistanceAccumCm = 0.0f;
	
};
