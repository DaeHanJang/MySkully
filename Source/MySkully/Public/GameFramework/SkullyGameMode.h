#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SkullyGameMode.generated.h"

UCLASS()
class MYSKULLY_API ASkullyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ASkullyGameMode();
	
	FORCEINLINE const FVector& GetSkullyRespawnLocation() const { return SkullyRespawnLocation; }
	FORCEINLINE void SetSkullyRespawnLocation(const FVector& Location) { SkullyRespawnLocation = Location; }
	FORCEINLINE uint8 GetSaveIndex() const { return SaveIndex; }
	FORCEINLINE void SetSaveIndex(uint8 Index) { SaveIndex = Index; }
	
	// Respawn
	void RespawnPlayer();
	
private:
	// Save
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	FVector SkullyRespawnLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	uint8 SaveIndex = 0;
	
};
