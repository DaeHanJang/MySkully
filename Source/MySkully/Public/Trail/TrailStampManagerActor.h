#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailStampManagerActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

UCLASS(NotBlueprintable)
class MYSKULLY_API ATrailStampManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATrailStampManagerActor();
	
	UHierarchicalInstancedStaticMeshComponent* GetHISM() const { return HISM; }
	
	// RVT 대상 설정(Receiver가 나중에 호출하거나, Subsystem이 직접 호출)
	void ConfigureForRVT(URuntimeVirtualTexture* InRVT);

protected:
	UPROPERTY()
	USceneComponent* Root = nullptr;
	
	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* HISM = nullptr;
	
};
