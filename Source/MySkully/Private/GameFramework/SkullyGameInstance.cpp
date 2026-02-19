#include "GameFramework/SkullyGameInstance.h"

#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

void USkullyGameInstance::PlayMusic(FName Channel, USoundBase* Sound, float Volume, float FadeIn)
{
	if (Sound == nullptr)
	{
		return;
	}

	if (MusicMap.Contains(Channel) == true)
	{
		if (MusicMap[Channel] != nullptr)
		{
			MusicMap[Channel]->FadeOut(0.2f, 0.0f);
			MusicMap[Channel]->Stop();
		}
		MusicMap.Remove(Channel);
	}

	UAudioComponent* NewComp = UGameplayStatics::SpawnSound2D(this, Sound, Volume, 1.0f, 0.0f, nullptr, true);

	if (NewComp != nullptr)
	{
		NewComp->FadeIn(FadeIn, Volume);
		MusicMap.Add(Channel, NewComp);
	}
}

void USkullyGameInstance::StopMusic(FName Channel, float FadeOut)
{
	if (MusicMap.Contains(Channel) == false)
	{
		return;
	}

	if (MusicMap[Channel] != nullptr)
	{
		MusicMap[Channel]->FadeOut(FadeOut, 0.0f);
	}

	MusicMap.Remove(Channel);
}

void USkullyGameInstance::StopAllMusic()
{
	for (auto& Pair : MusicMap)
	{
		if (Pair.Value != nullptr)
		{
			Pair.Value->FadeOut(0.5f, 0.0f);
		}
	}

	MusicMap.Empty();
}
