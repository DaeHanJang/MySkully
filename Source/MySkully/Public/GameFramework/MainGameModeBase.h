/*
 * 파일명: MainGameModeBase.h
 * 생성일: 2026-01-07
 * 수정일: 2026-01-09
 * 내용: 메인 화면 게임 모드
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameModeBase.generated.h"

class UUserWidget;

UCLASS()
class MYSKULLY_API AMainGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AMainGameModeBase();
	
public:
	UFUNCTION(BlueprintCallable, Category = "Option")
	void SetWidget(UUserWidget* widget);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Viewport")
	UUserWidget* currentWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport")
	TSubclassOf<UUserWidget> mainWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport")
	TSubclassOf<UUserWidget> optionWidget;
};
