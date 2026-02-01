#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrailManagerSubsystem.generated.h"

class ATrailStampManagerActor;
class UStaticMesh;
class UMaterialInterface;
class URuntimeVirtualTexture;

USTRUCT(BlueprintType)
struct FTrailStampRequest
{
	GENERATED_BODY()
	
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FVector Normal = FVector::UpVector;
	UPROPERTY() float Radius = 50.0f; // 반지름(cm)
	UPROPERTY() float Strength = 1.0f; // 바닥에 남는 강도(0~1)
};

USTRUCT(BlueprintType)
struct FTrailReceiverConfig
{
	GENERATED_BODY()
	
	// RVT 타겟(필수)
	UPROPERTY(EditAnywhere) TObjectPtr<URuntimeVirtualTexture> TrailRVT = nullptr;
	// 어떤 매니저 액터를 스폰할지
	UPROPERTY(EditAnywhere) TSubclassOf<ATrailStampManagerActor> ManagerClass = nullptr;
	// 스탬프 리소스
	UPROPERTY(EditAnywhere) TObjectPtr<UStaticMesh> StampMesh = nullptr;
	UPROPERTY(EditAnywhere) TObjectPtr<UMaterialInterface> StampMaterial = nullptr;
	//인스턴스 정책
	UPROPERTY(EditAnywhere) int32 MaxInstances = 2000;
	UPROPERTY(EditAnywhere) bool bReuseInstances = true;
};

UCLASS()
class MYSKULLY_API UTrailManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:	
	virtual void Deinitialize() override;
	
	// Receiver가 호출하는 메인 API
	void RegisterReceiver(AActor* Receiver, const FTrailReceiverConfig& Config);
	void UnregisterReceiver(AActor* Receiver);
	
	// 스탬프 요청
	void RequestStamp(const FTrailStampRequest& Req);
	
	void UpdateFades(float DeltaSeconds);
	
private:
	bool EnsureManagerActor();
	void DestroyManagerActor();
	void ApplyConfigToManager();
	FTransform MakeStampTransform(const FVector& Location, const FVector& Normal, float Radius) const;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<ATrailStampManagerActor> ManagerActor = nullptr;
	
	// 현재 활성 Receiver (스트리밍/중복 대비)
	TWeakObjectPtr<AActor> CurrentReceiver;
	
	// 활성 설정 캐시
	UPROPERTY(Transient) TObjectPtr<URuntimeVirtualTexture> TrailRVT = nullptr;
	UPROPERTY(Transient) TSubclassOf<ATrailStampManagerActor> ManagerClass = nullptr;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> StampMesh = nullptr;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> StampMaterial = nullptr;
	
	UPROPERTY()
	UMaterialInterface* Test = nullptr;
	
	UPROPERTY(EditAnywhere, Category="Trail")
	float SurfaceOffsetCm = 0.5f;
	UPROPERTY(EditAnywhere, Category="Fade")
	float TrailLifetimeSec = 2.5f; 
	
	int32 MaxInstances = 2000;
	bool bReuseInstances = true;
	
	// 원형 버퍼 인덱스
	int32 NextReuseIndex = 0;
	
	// 인스턴스별 나이
	TArray<float> InstanceAgesSec;
		
};
