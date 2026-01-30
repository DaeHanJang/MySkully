#include "Trail/TrailManagerSubsystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Trail/TrailStampManagerActor.h"

void UTrailManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	StampMesh = DefaultStampMesh;
	StampMaterial = DefaultStampMaterial;
	SetTrailRVT(DefaultTrailRVT);
	
	EnsureManagerActor();
}

void UTrailManagerSubsystem::Deinitialize()
{
	if (ManagerActor != nullptr)
	{
		ManagerActor->Destroy();
		ManagerActor = nullptr;
	}
	
	Super::Deinitialize();
}

bool UTrailManagerSubsystem::EnsureManagerActor()
{
	// 매니저가 있으면
	if (ManagerActor != nullptr)
	{
		return true;
	}
	// 월드 생성이 안됐으면 
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bHideFromSceneOutliner = true;
	Params.ObjectFlags |= RF_Transient; // 저장/패키징에 남지 않게
	// TrailStampManagerActor 생성
	ManagerActor = World->SpawnActor<ATrailStampManagerActor>(ATrailStampManagerActor::StaticClass(), Params);
	if (ManagerActor == nullptr)
	{
		return false;
	}
	
	// HISM 기본 세팅(메시/머티리얼)
	if (UHierarchicalInstancedStaticMeshComponent* HISM = ManagerActor->GetHISM())
	{
		if (StampMesh != nullptr)
		{
			HISM->SetStaticMesh(StampMesh);
		}
		if (StampMaterial != nullptr)
		{
			HISM->SetMaterial(0, StampMaterial);
		}
		
		// 인스턴스 커스텀 데이터(시간/정도)
		HISM->NumCustomDataFloats = 2;
	}
	
	// RVT 설정이 이미 들어와 있으면 반영
	if (TrailRVT != nullptr)
	{
		ManagerActor->ConfigureForRVT(TrailRVT);
	}
	
	return true;
}

void UTrailManagerSubsystem::SetTrailRVT(URuntimeVirtualTexture* InRVT)
{
	TrailRVT = InRVT;
	EnsureManagerActor();
	
	if (ManagerActor != nullptr && TrailRVT != nullptr)
	{
		ManagerActor->ConfigureForRVT(TrailRVT);
	}
}

void UTrailManagerSubsystem::RequestStamp(const FTrailStampRequest& Req)
{
	// 매니저/HISM 존재 확인
	if (EnsureManagerActor() == false)
	{
		return;
	}
	// RVT 타겟 없으면 찍지 않음
	if (TrailRVT == nullptr)
	{
		return;
	}
	
	UHierarchicalInstancedStaticMeshComponent* HISM = ManagerActor->GetHISM();
	if (HISM == nullptr)
	{
		return;
	}
	
	const FTransform Xform = MakeStampTransform(Req.Location, Req.Normal, Req.Radius);
	
	int32 InstanceIndex = INDEX_NONE;
	
	if (bReuseInstances == true && HISM->GetInstanceCount() >= MaxInstances)
	{
		// 재활용: 기존 인스턴스의 위치/회전/스케일만 갱신
		InstanceIndex = NextReuseIndex;
		HISM->UpdateInstanceTransform(InstanceIndex, Xform, true, true, true);
		
		NextReuseIndex = (NextReuseIndex + 1) % MaxInstances;
	}
	else
	{
		// 신규 추가
		InstanceIndex = HISM->AddInstance(Xform, true);
	}
	
	// 인스턴스별 커스텀 데이터 설정: [0]=TimeWrapped, [1]=Strength
	// 머티리얼에서 PerInstanceCustomData(0/1)로 읽는다.
	if (InstanceIndex != INDEX_NONE)
	{
		// SetCustomDataValue는 마지막 인자로 렌더 업데이트 여부가 있음
		HISM->SetCustomDataValue(InstanceIndex, 0, Req.TimeWrapped, true);
		HISM->SetCustomDataValue(InstanceIndex, 1, Req.Strength, true);
	}
}

FTransform UTrailManagerSubsystem::MakeStampTransform(const FVector& Location, const FVector& Normal,
	float Radius) const
{
	// 평면(Plane)의 위쪽(Z+)이 표면 노멀을 향하도록 회전
	const FQuat Rot = FRotationMatrix::MakeFromZ(Normal).ToQuat();
	
	// 스탬프 메쉬 크기에 따라 보정이 필요할 수 있음
	// 기본 Plane이 100x100이라면 Radius=50일 때 스케일 1이 맞는다
	const float Diameter = Radius * 2.0f;
	const FVector Scale(Diameter / 100.0f, Diameter / 100.0f, 1.0f);
	
	return FTransform(Rot, Location, Scale);
}
