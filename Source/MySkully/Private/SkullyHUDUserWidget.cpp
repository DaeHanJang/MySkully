#include "SkullyHUDUserWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void USkullyHUDUserWidget::SetCollectableMaxText(const uint8 Value) const
{
	CollectableMaxTextBlock->SetText(FText::AsNumber(Value));
}

void USkullyHUDUserWidget::UpdateCollectableText(const uint8 Value) const
{
	CollectableTextBlock->SetText(FText::AsNumber(Value));
}

void USkullyHUDUserWidget::UpdateHPProgressBar(const float Ratio) const
{
	LeftHPProgressBar->SetPercent(Ratio);
	RightHPProgressBar->SetPercent(Ratio);
}

void USkullyHUDUserWidget::SetHPImageColor(const FLinearColor& NewColor) const
{
	SkullyImage->SetColorAndOpacity(NewColor);
	LeftHPProgressBar->SetFillColorAndOpacity(NewColor);
	RightHPProgressBar->SetFillColorAndOpacity(NewColor);
}

void USkullyHUDUserWidget::SetDefaultHPImageColor() const
{
	SkullyImage->SetColorAndOpacity(FLinearColor::White);
	LeftHPProgressBar->SetFillColorAndOpacity(FLinearColor::White);
	RightHPProgressBar->SetFillColorAndOpacity(FLinearColor::White);
}

void USkullyHUDUserWidget::TakeDamage(const float Ratio)
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkullyHUDUserWidget][TakeDamage] World = nullptr"));
		return;
	}
	
	UpdateHPProgressBar(Ratio);
	SetHPImageColor(FLinearColor::Red);
	World->GetTimerManager().ClearTimer(DelayTimerHandle);
	World->GetTimerManager().SetTimer(DelayTimerHandle, this, &USkullyHUDUserWidget::SetDefaultHPImageColor, 0.0f, false, 0.5f);
}

void USkullyHUDUserWidget::Die() const
{
	UpdateHPProgressBar(0.0f);
	SkullyImage->SetBrushFromTexture(SkullyDeathIcon, true);
}
