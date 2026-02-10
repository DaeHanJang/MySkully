#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KnockOverBridge.generated.h"

class UBoxComponent;

UCLASS()
class MYSKULLY_API AKnockOverBridge : public AActor
{
	GENERATED_BODY()
	
public:	
	AKnockOverBridge();
	
	FORCEINLINE UBoxComponent* GetOverlapCollision() const { return OverlapCollision; }
	
	void KnockOver();
	
private:
	void UpdateKnockOverRotation();

private:
	// Scene Component(Root)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneComponent;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BridgeMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GateMesh;
	
	// Block Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BridgeCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> GateFloorCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> GateLeftCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> GateRightBottomCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> GateRightTopCollision;
	
	// Overlap Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> OverlapCollision;
	
	// Knock Over
	FTimerHandle KnockOverTimerHandle;
	float KnockOverElapsed = 0.f;
	float KnockOverDuration = 3.0f;
	float StartPitch = 90.f;
	
};
