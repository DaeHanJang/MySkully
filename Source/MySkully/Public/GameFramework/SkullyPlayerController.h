#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SkullyPlayerController.generated.h"

class USkullyHUDUserWidget;
class ASkully;

UCLASS()
class MYSKULLY_API ASkullyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:	
	FORCEINLINE ASkully* GetSkully() const { return Skully; }
	FORCEINLINE void SetSkully(ASkully* NewSkully) { Skully = NewSkully; }
	FORCEINLINE USkullyHUDUserWidget* GetHUDWidget() const { return HUDWidgetInstance; }
	
	void RequestShowCheckPointUI();
	
protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY();
	TObjectPtr<ASkully> Skully;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkullyHUDUserWidget> HUDWidgetInstance;
};
