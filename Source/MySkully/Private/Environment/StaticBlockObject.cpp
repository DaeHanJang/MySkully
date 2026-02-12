#include "Environment/StaticBlockObject.h"

#include "Components/BoxComponent.h"

AStaticBlockObject::AStaticBlockObject()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->PrimaryComponentTick.bCanEverTick = false;
	
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(CollisionComponent);
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComp->PrimaryComponentTick.bCanEverTick = false;
}
