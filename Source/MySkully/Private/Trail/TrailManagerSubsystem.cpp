#include "Trail/TrailManagerSubsystem.h"

#include "Trail/TrailStampManagerActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "VT/RuntimeVirtualTexture.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

void UTrailManagerSubsystem::Deinitialize()
{
	DestroyManagerActor();
	Super::Deinitialize();
}

void UTrailManagerSubsystem::RegisterReceiver(AActor* Receiver, const FTrailReceiverConfig& Config)
{
	// Receiver가 null이어도 테스트 편의를 위해 허용
	if (Receiver != nullptr)
	{
		CurrentReceiver = Receiver;
	}
	else
	{
		CurrentReceiver.Reset();
	}
	
	TrailRVT = Config.TrailRVT;
	ManagerClass = Config.ManagerClass;
	StampMesh = Config.StampMesh;
	StampMaterial = Config.StampMaterial;
	MaxInstances = Config.MaxInstances;
	bReuseInstances = Config.bReuseInstances;
	
	if (EnsureManagerActor() == true)
	{
		ApplyConfigToManager();
	}
}

void UTrailManagerSubsystem::UnregisterReceiver(AActor* Receiver)
{
	// 현재 활성 Receiver가 아닌 애가 해제하려 하면 무시
	if (CurrentReceiver.IsValid() == true && CurrentReceiver.Get() != Receiver)
	{
		return;
	}
	
	CurrentReceiver.Reset();
	
	// 정책: Receiver가 없어지면 트레일 시스템을 정지시키기
	TrailRVT = nullptr;
	StampMesh = nullptr;
	StampMaterial = nullptr;
	ManagerClass = nullptr;
	
	NextReuseIndex = 0;
	
	DestroyManagerActor();
}

bool UTrailManagerSubsystem::EnsureManagerActor()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	
	// 원하는 클래스 결정
	UClass* DesiredClass = ManagerClass != nullptr ? ManagerClass.Get() : ATrailStampManagerActor::StaticClass();
	
	// 이미 있고 클래스도 맞으면 OK
	if (ManagerActor != nullptr && ManagerActor->IsA(DesiredClass))
	{
		return true;
	}
	
	// 클래스가 바뀌었거나, 기존이 없으면 재생성
	DestroyManagerActor();
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bHideFromSceneOutliner = true;
	Params.ObjectFlags |= RF_Transient; // 저장/패키징에 남지 않게
	
	ManagerActor = World->SpawnActor<ATrailStampManagerActor>(DesiredClass, Params);
	if (ManagerActor == nullptr)
	{
		return false;
	}
	
	// 새로 만들었으니 재활용 인덱스 리셋
	NextReuseIndex = 0;
	InstanceAgesSec.Reset();
	
	ApplyConfigToManager();
	
	return true;
}


void UTrailManagerSubsystem::DestroyManagerActor()
{
	if (ManagerActor != nullptr)
	{
		ManagerActor->Destroy();
		ManagerActor = nullptr;
	}
}

void UTrailManagerSubsystem::ApplyConfigToManager()
{
	if (ManagerActor == nullptr)
	{
		return;
	}
	
	UHierarchicalInstancedStaticMeshComponent* HISM = ManagerActor->GetHISM();
	if (HISM == nullptr)
	{
		return;
	}
	
	HISM->SetMobility(EComponentMobility::Movable);
	// 커스템 데이터 사용 재확정(BP/재생성 꼬임 방지)
	HISM->NumCustomDataFloats = 2;
	
	// 메쉬 fallback 포함
	UStaticMesh* UseMesh = StampMesh;
	if (UseMesh == nullptr)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
		UseMesh = Plane.Succeeded() == true ? Plane.Object : nullptr;
	}
	
	const UStaticMesh* PrevMesh = HISM->GetStaticMesh();
	const UMaterialInterface* PrevMat0 = HISM->GetMaterial(0);
	
	if (UseMesh != nullptr)
	{
		HISM->SetStaticMesh(UseMesh);
	}
	if (StampMaterial != nullptr)
	{
		HISM->SetMaterial(0, StampMaterial);
	}
	
	const bool bMeshChanged = PrevMesh != HISM->GetStaticMesh();
	const bool bMatChanged = PrevMat0 != HISM->GetMaterial(0);
	
	if (bMeshChanged == true || bMatChanged == true)
	{
		HISM->ClearInstances();
		NextReuseIndex = 0;
		InstanceAgesSec.Reset();
	}
	
	// RVT 타겟
	HISM->RuntimeVirtualTextures.Empty();
	if (TrailRVT != nullptr)
	{
		HISM->RuntimeVirtualTextures.Add(TrailRVT);
		
		HISM->SetCastShadow(false); // 불필요한 그림자 제거
		
		// 혹시 매니저 액터 쪽에서 추가 설정이 더 있자면 유지
		ManagerActor->ConfigureForRVT(TrailRVT);
	}
	
	// 렌더 상태 갱신: 메시/머티리얼/RVT 변경 반영
	HISM->MarkRenderStateDirty();
}

void UTrailManagerSubsystem::RequestStamp(const FTrailStampRequest& Req)
{
	// RVT가 없으면 찍을 수 없음
	if (TrailRVT == nullptr)
	{
		return;
	}
	
	// 매니저 확보(설정이 들어온 이후만 생성됨)
	if (EnsureManagerActor() == false || ManagerActor == nullptr)
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
	
	const bool bCanReuse = bReuseInstances == true && MaxInstances > 0 && HISM->GetInstanceCount() >= MaxInstances;
	if (bCanReuse)
	{
		// 재활용: 기존 인스턴스의 위치/회전/스케일만 갱신
		InstanceIndex = NextReuseIndex;
		
		HISM->UpdateInstanceTransform(InstanceIndex, Xform, true, true, true);		
		NextReuseIndex = (NextReuseIndex + 1) % MaxInstances;
		
		// 재사용 인스턴스 Age 리셋
		if (InstanceAgesSec.IsValidIndex(InstanceIndex))
		{
			InstanceAgesSec[InstanceIndex] = 0.0f;
		}
	}
	else
	{
		// 신규 추가
		InstanceIndex = HISM->AddInstance(Xform, true);
		
		// 신규면 배열 크기 확장
		if (InstanceIndex != INDEX_NONE)
		{
			if (InstanceAgesSec.Num() <= InstanceIndex)
			{
				InstanceAgesSec.SetNum(InstanceIndex + 1);
			}
			InstanceAgesSec[InstanceIndex] = 0.0f;
		}
	}
	
	// 인스턴스별 커스텀 데이터 설정: [0]=FadeAlpha, [1]=Strength
	// 머티리얼에서 PerInstanceCustomData(0/1)로 읽는다.
	if (InstanceIndex != INDEX_NONE)
	{
		// SetCustomDataValue는 마지막 인자로 렌더 업데이트 여부가 있음
		HISM->SetCustomDataValue(InstanceIndex, 0, 1.0f, true);
		HISM->SetCustomDataValue(InstanceIndex, 1, Req.Strength, true);
	}
}

FTransform UTrailManagerSubsystem::MakeStampTransform(const FVector& Location, const FVector& Normal, float Radius) const
{
	const FVector N = Normal.IsNearlyZero() == true ? FVector::UpVector : Normal.GetSafeNormal();
	const FQuat Rot = FRotationMatrix::MakeFromZ(N).ToQuat(); // 평면(Plane)의 위쪽(Z+)이 표면 노멀을 향하도록 회전
	
	// 스탬프 메쉬 크기에 따라 보정이 필요할 수 있음
	// 기본 Plane이 100x100이라고 가정(UE 기본 Plane)
	const float Diameter = Radius * 2.0f;
	const FVector Scale(Diameter / 100.0f, Diameter / 100.0f, 1.0f);
	
	// 바닥에서 살짝 띄우기 (메시 피벗이 중앙이라 절반이 파묻히는 현상 방지)
	const FVector P = Location + N * SurfaceOffsetCm;
	
	return FTransform(Rot, P, Scale);
}

void UTrailManagerSubsystem::UpdateFades(float DeltaSeconds)
{
	if (ManagerActor == nullptr)
	{
		return;
	}
	
	UHierarchicalInstancedStaticMeshComponent* HISM = ManagerActor->GetHISM();
	if (HISM == nullptr)
	{
		return;
	}

	const int32 Count = HISM->GetInstanceCount();
	if (Count <= 0)
	{
		return;
	}

	if (TrailLifetimeSec <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 배열 크기 보정(혹시 모를 방어)
	if (InstanceAgesSec.Num() < Count)
	{
		InstanceAgesSec.SetNum(Count);
	}

	bool bAnyUpdated = false;

	for (int32 i = 0; i < Count; ++i)
	{
		InstanceAgesSec[i] += DeltaSeconds;

		float Fade = 1.0f - (InstanceAgesSec[i] / TrailLifetimeSec);
		Fade = FMath::Clamp(Fade, 0.0f, 1.0f);

		// 렌더 업데이트 플래그는 마지막에 한번만 true 주기 위해 false로
		HISM->SetCustomDataValue(i, 0, Fade, false);
		bAnyUpdated = true;
	}

	if (bAnyUpdated == true)
	{
		// 마지막에 한 번만 렌더 갱신
		HISM->MarkRenderStateDirty();
	}
}
