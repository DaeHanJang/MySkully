#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SkullyPlayerController.generated.h"

class ASkully;

UCLASS()
class MYSKULLY_API ASkullyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	FORCEINLINE ASkully* GetSkully() const { return Skully; }
	FORCEINLINE void SetSkully(ASkully* NewSkully) { Skully = NewSkully; }
		
private:
	UPROPERTY();
	TObjectPtr<ASkully> Skully;
	
};
