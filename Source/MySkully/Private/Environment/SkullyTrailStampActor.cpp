#include "Environment/SkullyTrailStampActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

ASkullyTrailStampActor::ASkullyTrailStampActor()
{
 	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorEnableCollision(false);
}

void ASkullyTrailStampActor::EnsureHISM()
{
	if (StampHISM != nullptr)
	{
		return;
	}
	
	StampHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, TEXT("TrailStampHISM"));
	StampHISM->SetupAttachment(RootComponent);
	AddInstanceComponent(StampHISM);
	StampHISM->RegisterComponent();

	StampHISM->SetMobility(EComponentMobility::Movable);
	StampHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StampHISM->SetCastShadow(false);
	StampHISM->bCastDynamicShadow = false;

	// 메쉬 자체는 화면에 안 보이게 (RVT에만 쓰기)
	StampHISM->SetRenderInMainPass(true);
	StampHISM->SetRenderInDepthPass(false);

	// RVT 패스에 항상 그리기
	StampHISM->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;

	// 인스턴싱/가시성 갱신 안정화용
	StampHISM->SetVisibility(true, true);
	StampHISM->bAffectDistanceFieldLighting = false;
	StampHISM->bAffectDynamicIndirectLighting =  false;
	StampHISM->bUseAttachParentBound = false;
	
	// (디버그용) 혹시 컬링/바운드 이슈면 임시로 키워서 확인
	StampHISM->SetBoundsScale(5.0f);
	
	StampHISM->MarkRenderStateDirty(); // 렌더 상태 갱신
}

void ASkullyTrailStampActor::Initialize(UStaticMesh* InStampMesh, UMaterialInterface* InStampMaterial,
	URuntimeVirtualTexture* InTargetRVT, int32 InMaxInstances, bool bInWriteCustomData)
{
	UE_LOG(LogTemp, Warning, TEXT("[Trail Init] Mesh=%s Mat=%s"),
	*GetNameSafe(InStampMesh), *GetNameSafe(InStampMaterial));
	
	EnsureHISM();

	MaxInstances = FMath::Max(1, InMaxInstances);
	bWriteCustomData = bInWriteCustomData;

	if (InStampMesh != nullptr)
	{
		StampHISM->SetStaticMesh(InStampMesh);
	}
	if (InStampMaterial != nullptr)
	{
		StampHISM->SetMaterial(0, InStampMaterial);
	}
	
	StampHISM->RuntimeVirtualTextures.Reset();
	if (InTargetRVT != nullptr)
	{
		StampHISM->RuntimeVirtualTextures.Add(InTargetRVT);
	}

	StampHISM->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
	StampHISM->SetRenderInMainPass(true);
	StampHISM->SetRenderInDepthPass(false);
	
	StampHISM->NumCustomDataFloats = bWriteCustomData ? 2 : 0;
	
	StampHISM->MarkRenderStateDirty();
}

int32 ASkullyTrailStampActor::AddOrUpdateStamp(const FTransform& WorldXform, float SpawnTimeSeconds, float Strength)
{
	UE_LOG(LogTemp, Warning, TEXT("[Stamp] Count=%d"), 
	StampHISM ? StampHISM->GetInstanceCount() : -1);
	
	EnsureHISM();
	
	if (StampHISM == nullptr)
	{
		return INDEX_NONE;
	}

	const int32 Count = StampHISM->GetInstanceCount();
	int32 InstanceIndex = INDEX_NONE;

	if (Count < MaxInstances)
	{
		InstanceIndex = StampHISM->AddInstance(WorldXform, true);
		NextSlot = Count + 1;
	}
	else
	{
		InstanceIndex = NextSlot % MaxInstances;
		StampHISM->UpdateInstanceTransform(InstanceIndex, WorldXform, true, true, true);
		++NextSlot;
	}
	
	if (bWriteCustomData && InstanceIndex != INDEX_NONE)
	{
		StampHISM->SetCustomDataValue(InstanceIndex, 0, SpawnTimeSeconds, false);
		StampHISM->SetCustomDataValue(InstanceIndex, 1, Strength, false);
		StampHISM->MarkRenderStateDirty();
	}

	return InstanceIndex;
}
