#include "Trail/TrailStampManagerActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Trail/TrailManagerSubsystem.h"
#include "VT/RuntimeVirtualTexture.h"

ATrailStampManagerActor::ATrailStampManagerActor()
{
 	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	HISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TrailHISM"));
	HISM->SetupAttachment(Root);
	
	HISM->SetMobility(EComponentMobility::Movable);
	
	// 스탬프 메시를 화면에 직접 보여줄 필요 없음(RVT에만 기록)
	HISM->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
	
	// 메인 패스/뎁스 패스에서 렌더 금지
	HISM->SetRenderInMainPass(true);
	HISM->SetRenderInDepthPass(false);
	
	// 렌더링 부가 기능 끄기(가볍게)
	HISM->SetCastShadow(false);
	HISM->bAffectDistanceFieldLighting = false;
	
	// PerInstanceCustomData: [0]=fade, [1]=strength
	HISM->NumCustomDataFloats = 2;
	
	// 물리/트레이스 간섭 금지
	HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISM->SetGenerateOverlapEvents(false);
	HISM->SetCollisionResponseToAllChannels(ECR_Ignore);;
	
	// 내비게이션 영향 제거
	HISM->SetCanEverAffectNavigation(false);
}

void ATrailStampManagerActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (UWorld* World = GetWorld())
	{
		TrailMgr = World->GetSubsystem<UTrailManagerSubsystem>();
	}
}

void ATrailStampManagerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (TrailMgr.IsValid() == true)
	{
		TrailMgr->UpdateFades(DeltaSeconds);
	}
}


void ATrailStampManagerActor::ConfigureForRVT(URuntimeVirtualTexture* InRVT)
{
	if (HISM == nullptr || InRVT == nullptr)
	{
		return;
	}
	
	// 디테일 패널의 Draw in Virtual Textures 배열 세팅과 동일
	HISM->RuntimeVirtualTextures.Reset();
	HISM->RuntimeVirtualTextures.Add(InRVT);
	
	// 품질/성능 관련 값은 기본값 유지가 원칙이지만, 
	// 트레일은 작은 스탬프가 많으니 최소 커버리지는 0으로 두는 편이 안전
	HISM->VirtualTextureMinCoverage = 0;
	
	// 런타임에 RVT 바꾼 걸 렌더러에 반영
	HISM->MarkRenderStateDirty();
}

void ATrailStampManagerActor::ClearRVT()
{
	if (HISM == nullptr)
	{
		return;
	}
	
	HISM->RuntimeVirtualTextures.Reset();
	HISM->MarkRenderStateDirty();
}
