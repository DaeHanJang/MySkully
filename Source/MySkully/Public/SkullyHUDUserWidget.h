#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkullyHUDUserWidget.generated.h"

class USizeBox;
class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class MYSKULLY_API USkullyHUDUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitUI();
	void SetCollectableMaxText(const uint8 Value) const;
	void UpdateCollectableText(const uint8 Value) const;
	void TakeDamage(const float Ratio);
	void Die() const;
	void ShowCheckPointUI();
	void UpdateHPProgressBar(const float Ratio) const;
	void ShowResultUI(const uint8 Score, const uint8 DeathCount);
	
private:
	void SetHPImageColor(const FLinearColor& NewColor) const;
	void SetDefaultHPImageColor() const;
	void UpdateHiddenCheckPointUI();
	
public:
	// Collectable UI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CollectableMaxTextBlock;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CollectableTextBlock;
	
	// HP UI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkullyImage;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LeftHPProgressBar;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RightHPProgressBar;
	
	// CheckPoint UI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> CheckPointSizeBox;
	
	// Result UI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> ResultSizeBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreTextBlock;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultTextBlock;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkullyIcon")
	TObjectPtr<UTexture2D> SkullyIcon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkullyIcon")
	TObjectPtr<UTexture2D> SkullyDeathIcon;
	
	FTimerHandle DelayTimerHandle;
	
	FTimerHandle HiddenCheckPointTimerHandle;
};
