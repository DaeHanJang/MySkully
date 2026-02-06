#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClayMoundReactiveComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYSKULLY_API UClayMoundReactiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UClayMoundReactiveComponent();
	
	FORCEINLINE bool GetOnClayMoundSurface() const { return bOnClayMoundSurface; }
	void SetOnClayMoundSurface(const bool Value, const FVector& SurfaceLocation);
	FORCEINLINE const FVector& GetClayMoundSurfaceLocation() const { return ClayMoundSurfaceLocation; }
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClayMound", meta = (AllowPrivateAccess = true))
	bool bOnClayMoundSurface = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClayMound", meta = (AllowPrivateAccess = true))
	FVector ClayMoundSurfaceLocation = FVector::ZeroVector;
	
};
