#include "Components/HealthComponent/HealthComponent.h"

#include "Components/HealthComponent/HealthInterface.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UHealthComponent::LoseHealth(float Amount)
{
	Health -= Amount;
	Health = FMath::Max(Health, 0.0f);
	
	if (GetOwner()->Implements<UHealthInterface>())
	{
		IHealthInterface::Execute_OnTakeDamage(GetOwner());
	}
	
	if (Health <= 0.0f)
	{
		if (GetOwner()->Implements<UHealthInterface>())
		{
			IHealthInterface::Execute_OnDeath(GetOwner());
		}
	}
}

void UHealthComponent::GainHealth()
{
	Health += HealAmount;
	Health = FMath::Min(Health, 100.0f);

	if (GetOwner()->Implements<UHealthInterface>())
	{
		IHealthInterface::Execute_OnTakeHealth(GetOwner());
	}
}

