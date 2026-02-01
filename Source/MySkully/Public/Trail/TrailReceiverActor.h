#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailReceiverActor.generated.h"

class ARuntimeVirtualTextureVolume;
class ATrailStampManagerActor;
class UStaticMesh;
class UMaterialInterface;
class URuntimeVirtualTexture;

UCLASS()
class MYSKULLY_API ATrailReceiverActor : public AActor
{
	GENERATED_BODY()
	
public:
	ATrailReceiverActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	TObjectPtr<URuntimeVirtualTexture> TrailRVT = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	TObjectPtr<ARuntimeVirtualTextureVolume> RVTVolume = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	TSubclassOf<ATrailStampManagerActor> StampManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	TObjectPtr<UStaticMesh> StampMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	TObjectPtr<UMaterialInterface> StampMaterial;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	int32 MaxInstances = 2000;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail|Receiver")
	bool bReuseInstance = true;
	
private:
	bool bRegistered = false;

};
