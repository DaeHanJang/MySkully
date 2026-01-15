// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFramework/MySkullyGameInstance.h"
#include "GameFramework/GameUserSettings.h"

UMySkullyGameInstance::UMySkullyGameInstance()
{
}

void UMySkullyGameInstance::Init()
{
	Super::Init();
	
	// GameUserSettings에서 설정된 해상도 너비, 높이 값 가져오기
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	FIntPoint Resolution = Settings->GetScreenResolution();
	viewportWidth = Resolution.X;
	viewportHeight = Resolution.Y;
}
