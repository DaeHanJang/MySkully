#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ClayMoundReactiveInterface.generated.h"

UINTERFACE(MinimalAPI)
class UClayMoundReactiveInterface : public UInterface
{
	GENERATED_BODY()
};

class MYSKULLY_API IClayMoundReactiveInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "ClayMound")
	void OnEnterClayMound();
	virtual void OnEnterClayMound_Implementation() = 0;
	UFUNCTION(BlueprintNativeEvent, Category = "ClayMound")
	void OnExitClayMound();
	virtual void OnExitClayMound_Implementation() = 0;
	
};
