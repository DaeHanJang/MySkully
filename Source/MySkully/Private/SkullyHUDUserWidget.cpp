#include "SkullyHUDUserWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void USkullyHUDUserWidget::InitUI()
{
	UpdateHPProgressBar(1.0f);
	SkullyImage->SetBrushFromTexture(SkullyIcon, true);
	SkullyImage->SetDesiredSizeOverride(FVector2D(104.0f, 105.0f));
	SetHPImageColor(FLinearColor::White);
}

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
	World->GetTimerManager().SetTimer(DelayTimerHandle, this, &USkullyHUDUserWidget::SetDefaultHPImageColor, 0.5f, false);
}

void USkullyHUDUserWidget::Die() const
{
	UpdateHPProgressBar(0.0f);
	SkullyImage->SetBrushFromTexture(SkullyDeathIcon, true);
	SkullyImage->SetDesiredSizeOverride(FVector2D(104.0f, 105.0f));
}

void USkullyHUDUserWidget::ShowCheckPointUI()
{
	CheckPointSizeBox->SetVisibility(ESlateVisibility::Visible);
	CheckPointSizeBox->SetRenderOpacity(1.0f);
	GetWorld()->GetTimerManager().ClearTimer(HiddenCheckPointTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(HiddenCheckPointTimerHandle, this, &USkullyHUDUserWidget::UpdateHiddenCheckPointUI , 0.01f, true, 1.0f);
}

void USkullyHUDUserWidget::UpdateHiddenCheckPointUI()
{
	CheckPointSizeBox->SetRenderOpacity(CheckPointSizeBox->GetRenderOpacity() - 0.01f);
	if (CheckPointSizeBox->GetRenderOpacity() <= 0.0f)
	{
		CheckPointSizeBox->SetRenderOpacity(0.0f);
		CheckPointSizeBox->SetVisibility(ESlateVisibility::Hidden);
		GetWorld()->GetTimerManager().ClearTimer(HiddenCheckPointTimerHandle);
	}
}

void USkullyHUDUserWidget::ShowResultUI(const uint8 Score, const uint8 DeathCount)
{
	ScoreTextBlock->SetText(FText::Format(FText::FromString("Score: {0}"), FText::AsNumber(Score)));
	ResultTextBlock->SetText(FText::Format(FText::FromString("DeathCount: {0}"), FText::AsNumber(DeathCount)));
	ResultSizeBox->SetVisibility(ESlateVisibility::Visible);
}