#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrailManagerSubsystem.generated.h"

class ATrailStampManagerActor;

USTRUCT(BlueprintType)
struct FTrailStampRequest
{
	GENERATED_BODY()
	
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FVector Normal = FVector::UpVector;
	
	// 지름 대신 반지름/스케일로 쓰기 쉬워서 Radius로
	UPROPERTY() float Radius = 50.0f;
	// 바닥에 남는 강도(0~1)
	UPROPERTY() float Strength = 1.0f;
	// 0~1래핑된 시간값(페이드용)
	UPROPERTY() float TimeWrapped = 0.0f;
};

UCLASS()
class MYSKULLY_API UTrailManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// RVT/자원 설정(Receiver가 호출)
	void SetTrailRVT(URuntimeVirtualTexture* InRVT);
	
	// 스탬프 요청
	void RequestStamp(const FTrailStampRequest& Req);
	
public:
	UPROPERTY(EditDefaultsOnly)
	URuntimeVirtualTexture* DefaultTrailRVT;
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* DefaultStampMesh;
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DefaultStampMaterial;

private:
	// 내부 헬퍼
	// 필요할 때 매니저가 없으면 만들고, 있으면 그대로 사용
	bool EnsureManagerActor();
	FTransform MakeStampTransform(const FVector& Location, const FVector& Normal, float Radius) const;
	
private:
	UPROPERTY()
	ATrailStampManagerActor* ManagerActor = nullptr;
	
	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> TrailRVT = nullptr;
	
	// 스탬프 메시/머티리얼
	UPROPERTY()
	TObjectPtr<UStaticMesh> StampMesh = nullptr;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> StampMaterial = nullptr;
	
	// 간단한 재활용(원형 버퍼)
	int32 MaxInstances = 2000;
	int32 NextReuseIndex = 0;
	bool bReuseInstances = true;
		
};
