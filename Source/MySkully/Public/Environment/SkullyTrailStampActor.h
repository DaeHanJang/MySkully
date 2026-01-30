#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkullyTrailStampActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class MYSKULLY_API ASkullyTrailStampActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ASkullyTrailStampActor();
	
	UFUNCTION(BlueprintCallable)
	void Initialize(UStaticMesh* InStampMesh, UMaterialInterface* InStampMaterial, URuntimeVirtualTexture* InTargetRVT, int32 InMaxInstances, bool bInWriteCustomData);
	UFUNCTION(BlueprintCallable)
	int32 AddOrUpdateStamp(const FTransform& WorldXform, float SpawnTimeSeconds, float Strength);
	UFUNCTION(BlueprintCallable)
	UHierarchicalInstancedStaticMeshComponent* GetStampHISM() const { return StampHISM; }
	
private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> StampHISM = nullptr;
	
	int32 MaxInstances = 1024;
	int32 NextSlot = 0;
	bool bWriteCustomData = true;
	
	void EnsureHISM();
};
