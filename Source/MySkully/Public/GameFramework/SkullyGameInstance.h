#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SkullyGameInstance.generated.h"

UCLASS()
class MYSKULLY_API USkullyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void PlayMusic(FName Channel, USoundBase* Sound, float Volume = 1.0f, float FadeIn = 0.5f);

	UFUNCTION(BlueprintCallable)
	void StopMusic(FName Channel, float FadeOut = 0.5f);

	UFUNCTION(BlueprintCallable)
	void StopAllMusic();
	
private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UAudioComponent>> MusicMap;
	
};
