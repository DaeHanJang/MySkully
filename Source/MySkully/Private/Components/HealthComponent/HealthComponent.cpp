#include "Components/HealthComponent/HealthComponent.h"

#include "Components/HealthComponent/HealthInterface.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UHealthComponent::LoseHealth(const float Amount)
{
	Health -= Amount;
	Health = FMath::Max(Health, 0.0f);
	
	const bool bHasHealth = GetOwner()->Implements<UHealthInterface>();
	if (bHasHealth == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthComponent.cpp][LoseHealth] HealthInterface = nullptr"));
		return;
	}
	
	IHealthInterface::Execute_OnTakeDamage(GetOwner());
	if (Health <= 0.0f)
	{
		IHealthInterface::Execute_OnDeath(GetOwner());
	}
}

void UHealthComponent::GainHealth()
{
	Health += HealAmount;
	Health = FMath::Min(Health, 100.0f);

	const bool bHasHealth = GetOwner()->Implements<UHealthInterface>();
	if (bHasHealth == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthComponent.cpp][LoseHealth] HealthInterface = nullptr"));
		return;
	}
	
	IHealthInterface::Execute_OnTakeHealth(GetOwner());
}
