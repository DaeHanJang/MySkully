#include "Environment/StaticOverlapObject.h"

AStaticOverlapObject::AStaticOverlapObject()
{
	PrimaryActorTick.bCanEverTick = false;
	
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComp");
	SetRootComponent(StaticMeshComp);
	StaticMeshComp->SetCollisionProfileName(TEXT("OverlapAll"));
	StaticMeshComp->PrimaryComponentTick.bCanEverTick = false;
}
