#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SkullyGameMode.generated.h"

class UNiagaraSystem;
class USkullyHUDUserWidget;
class USoundCue;

UCLASS()
class MYSKULLY_API ASkullyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:	
	FORCEINLINE const FVector& GetSkullyRespawnLocation() const { return SkullyRespawnLocation; }
	FORCEINLINE void SetSkullyRespawnLocation(const FVector& Location) { SkullyRespawnLocation = Location; }
	FORCEINLINE uint8 GetSaveIndex() const { return SaveIndex; }
	FORCEINLINE void SetSaveIndex(const uint8 Index) { SaveIndex = Index; }
	FORCEINLINE uint8 GetScore() const { return Score; }
	FORCEINLINE void SetScore(const uint8 Value) { Score = Value;}
	FORCEINLINE void SetHUD(USkullyHUDUserWidget* NewHUD) { HUD = NewHUD; }
	FORCEINLINE uint8 GetDeathCount() const { return DeathCount; }
	
	// Save
	void RespawnPlayer();
	
	// Score
	void AddScore(const uint8 Value);
	
protected:
	virtual void BeginPlay() override;
	
private:
	// Save
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
	FVector SkullyRespawnLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
	uint8 SaveIndex = 0;
	
	// Score
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score", meta = (AllowPrivateAccess = "true"))
	uint8 Score = 0;
	
	// UI
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkullyHUDUserWidget> HUD;
	
	// Sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundWave> BGM;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundWave> Bird;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundCue> Wave;
	
	// Death Count
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
	uint8 DeathCount = 0;
	
	// Effect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> Effect;
	
};
