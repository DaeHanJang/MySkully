#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailStampManagerActor.generated.h"

class UTrailManagerSubsystem;
class UHierarchicalInstancedStaticMeshComponent;
class URuntimeVirtualTexture;

UCLASS()
class MYSKULLY_API ATrailStampManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATrailStampManagerActor();
	
	UHierarchicalInstancedStaticMeshComponent* GetHISM() const { return HISM; }
	
	// RVT 대상 설정: HISM이 RVT에만 기록하도록 구성
	void ConfigureForRVT(URuntimeVirtualTexture* InRVT);
	
	// RVT 해제
	void ClearRVT();
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = "Trail")
	TObjectPtr<USceneComponent> Root = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category = "Trail")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISM = nullptr;
	
	TWeakObjectPtr<UTrailManagerSubsystem> TrailMgr;
	
};
