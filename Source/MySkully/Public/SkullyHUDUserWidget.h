#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkullyHUDUserWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class MYSKULLY_API USkullyHUDUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:	
	void SetCollectableMaxText(const uint8 Value) const;
	void UpdateCollectableText(const uint8 Value) const;
	void TakeDamage(const float Ratio);
	void Die() const;
	
private:
	void UpdateHPProgressBar(const float Ratio) const;
	void SetHPImageColor(const FLinearColor& NewColor) const;
	void SetDefaultHPImageColor() const;
	
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
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkullyIcon")
	TObjectPtr<UTexture2D> SkullyDeathIcon;
	
	FTimerHandle DelayTimerHandle;
};
