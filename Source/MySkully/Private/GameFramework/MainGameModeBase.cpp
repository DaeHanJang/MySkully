/*
* 파일명: MainGameModeBase.h
 * 생성일: 2026-01-07
 * 수정일: 2026-01-09
 * 내용: 메인 화면 게임 모드
 */

#include "GameFramework/MainGameModeBase.h"
#include "Blueprint/UserWidget.h"

AMainGameModeBase::AMainGameModeBase()
{
}

void AMainGameModeBase::SetWidget(UUserWidget* widget)
{
	if (currentWidget != nullptr)
	{
		currentWidget->RemoveFromParent();
	}
	
	currentWidget = widget;
	currentWidget->AddToViewport();
}
