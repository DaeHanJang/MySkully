
#include "Trail/TrailStampManagerActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "VT/RuntimeVirtualTexture.h"

ATrailStampManagerActor::ATrailStampManagerActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	HISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TrailHISM"));
	HISM->SetupAttachment(Root);
	
	// 트레일은 화면에 스탬프 메시가 보일 필요가 없음(=RVT에만 쓰기)
	// UE 디테일의 Draw in Main Pass = Never에 해당
	HISM->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Never;
	
	// 인스턴스별로 2개의 float 커스텀 데이터(시간, 강도) 전달
	HISM->NumCustomDataFloats = 2;
}

void ATrailStampManagerActor::ConfigureForRVT(URuntimeVirtualTexture* InRVT)
{
	if (HISM == nullptr || InRVT == nullptr)
	{
		return;
	}
	
	// 디테일 패널의 Draw in Virtual Textures 배열에 RVT를 넣는 것과 동일
	HISM->RuntimeVirtualTextures.Reset();
	HISM->RuntimeVirtualTextures.Add(InRVT);
	
	// 성능/품질 관리 옵션
	// Mip 레벨 성정: 트레일은 디테일이 중요하므로, 최고 품질 기준을 시작
	HISM->VirtualTextureLodBias = 0;
	// 이 값보다 낮은 해상도 Mip애는 아예 기록하지 않음: 멀리 있는 트레일이 점점 사라짐
	HISM->VirtualTextureCullMips = 0;
	// 스탬프가 RVT 타일에서 차지하는 최소 면적 비율: 0=크기 상관없이 전부 기록, 0.01: 타일의 1%보다 작으면 기록 안 함
	HISM->VirtualTextureMinCoverage = 0;
}
