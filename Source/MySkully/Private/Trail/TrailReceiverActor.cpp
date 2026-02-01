#include "Trail/TrailReceiverActor.h"

#include "Trail/TrailManagerSubsystem.h"
#include "Engine/World.h"
#include "VT/RuntimeVirtualTexture.h"

ATrailReceiverActor::ATrailReceiverActor()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ATrailReceiverActor::BeginPlay()
{
	Super::BeginPlay();
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	UTrailManagerSubsystem* TrailMgr = World->GetSubsystem<UTrailManagerSubsystem>();
	if (TrailMgr == nullptr)
	{
		return;
	}
	
	if (TrailRVT == nullptr)
	{
		return;
	}
	if (StampMesh == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TrailReceiver] StampMesh is null. (Will fallback to Engine Plane)"));
	}
	if (StampMaterial == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TrailReceiver] StampMaterial is null. RVT write will likely fail."));
	}
	
	FTrailReceiverConfig Config;
	Config.TrailRVT = TrailRVT;
	Config.ManagerClass = StampManagerClass;
	Config.StampMesh = StampMesh;
	Config.StampMaterial = StampMaterial;
	Config.MaxInstances = MaxInstances;
	Config.bReuseInstances = bReuseInstance;
	
	TrailMgr->RegisterReceiver(this, Config);
	bRegistered = true;
}

void ATrailReceiverActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRegistered == true)
	{
		if (UWorld* World = GetWorld())
		{
			if (UTrailManagerSubsystem* TrailMgr = World->GetSubsystem<UTrailManagerSubsystem>())
			{
				TrailMgr->UnregisterReceiver(this);
			}
		}
	}
	
	bRegistered = false;
	
	Super::EndPlay(EndPlayReason);
}
