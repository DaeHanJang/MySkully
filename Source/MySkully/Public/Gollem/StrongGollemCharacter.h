#pragma once

#include "CoreMinimal.h"
#include "Components/HealthComponent/HealthInterface.h"
#include "Gollem/GollemCharacter.h"
#include "StrongGollemCharacter.generated.h"

UCLASS()
class MYSKULLY_API AStrongGollemCharacter : public AGollemCharacter, public IHealthInterface
{
	GENERATED_BODY()
	
public:
	AStrongGollemCharacter();
	
	// Camera Box Component
	UFUNCTION()
	virtual void OnCameraBoxComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
								   int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	virtual void OnCameraBoxComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
protected:
	virtual void BeginPlay() override;
		
	virtual void OnTakeDamage_Implementation() override;
	virtual void OnDeath_Implementation() override;
	virtual void OnTakeHealth_Implementation() override;
	
	virtual void InteractAction_Implementation() override;
	virtual void PrimaryAction_Implementation() override;
	virtual void SecondaryAction_Implementation() override;
	
};
