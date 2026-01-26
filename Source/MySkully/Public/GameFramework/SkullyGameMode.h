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
	
	// Save
	FORCEINLINE FVector GetSaveLocation() const { return SaveLocation; }
	FORCEINLINE void SetSaveLocation(const FVector& Location) { SaveLocation = Location; }
	FORCEINLINE uint8 GetSaveIndex() const { return SaveIndex; }
	FORCEINLINE void SetSaveIndex(uint8 Index) { SaveIndex = Index; }
	
	// Rewpawn
	void RespawnPlayer();
	
private:
	// Save
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	FVector SaveLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess="true"))
	uint8 SaveIndex = 0;
};
