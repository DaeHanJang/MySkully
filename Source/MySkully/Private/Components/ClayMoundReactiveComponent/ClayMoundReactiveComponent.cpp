#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveComponent.h"

#include "Components/ClayMoundReactiveComponent/ClayMoundReactiveInterface.h"

UClayMoundReactiveComponent::UClayMoundReactiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UClayMoundReactiveComponent::SetOnClayMoundSurface(const bool Value, const FVector& SurfaceLocation)
{
	bOnClayMoundSurface = Value;
	
	const bool bHasClayMoundReactive = GetOwner()->Implements<UClayMoundReactiveInterface>();
	if (bHasClayMoundReactive == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ClayMoundReactiveComponent.cpp][SetClayMound] ClayMoundReactiveInterface = nullptr"));
		return;
	}
	
	if (bOnClayMoundSurface == true)
	{
		ClayMoundSurfaceLocation = SurfaceLocation + FVector(0.0f, 0.0f, 100.0f);
		IClayMoundReactiveInterface::Execute_OnEnterClayMound(GetOwner());
	}
	else
	{
		IClayMoundReactiveInterface::Execute_OnExitClayMound(GetOwner());
	}
}
