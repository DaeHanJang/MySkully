// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MySkullyGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MYSKULLY_API UMySkullyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UMySkullyGameInstance();
	
protected:
	virtual void Init() override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Option")
	int32 viewportWidth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Option")
	int32 viewportHeight;
	
};
