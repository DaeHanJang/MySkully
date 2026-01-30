#include "Skully/SkullyTrailComponent.h"

#include "Environment/SkullyTrailStampActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Skully/SkullyMovementComponent.h"

USkullyTrailComponent::USkullyTrailComponent()
{
	// Movement 델리게이트 기반이라 Tick 불필요
	PrimaryComponentTick.bCanEverTick = false;
}

void USkullyTrailComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("[Trail Init] Component Mesh=%s Mat=%s"),
	*GetNameSafe(StampMesh), *GetNameSafe(StampMaterial));
	
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}
	
	MoveComp = Owner->FindComponentByClass<USkullyMovementComponent>();
	if (MoveComp == nullptr)
	{
		return;
	}
	
	// 월드 고정 스탬프 액터 생성
	if (StampActorClass == nullptr)
	{
		StampActorClass = ASkullyTrailStampActor::StaticClass();
	}
	
	UWorld* World = GetWorld();
	if (World != nullptr && StampActor == nullptr)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		StampActor = World->SpawnActor<ASkullyTrailStampActor>(StampActorClass, FTransform::Identity, Params);
		if (StampActor != nullptr)
		{
			StampActor->Initialize(StampMesh, StampMaterial, TargetRVT, MaxInstances, bWriteSpawnTimeToCustomData);
		}
	}
	
	// Movement 델리게이트 구독
	MoveComp->OnMovementChanged.AddUObject(this, &USkullyTrailComponent::HandleMovementChanged);
	
	UE_LOG(LogTemp, Warning, TEXT("[Trail] StampActor=%s Mesh=%s Mat=%s RVTptr=%p"),
	*GetNameSafe(StampActor),
	*GetNameSafe(StampMesh),
	*GetNameSafe(StampMaterial),
	TargetRVT);
}

void USkullyTrailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MoveComp != nullptr)
	{
		MoveComp->OnMovementChanged.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void USkullyTrailComponent::HandleMovementChanged(float DeltaTime, float Speed2D, FVector Dir2D)
{
	if (bEnableTrail == false)
	{
		return;
	}
	if (MoveComp == nullptr || StampActor == nullptr)
	{
		return;
	}
	if (TargetRVT == nullptr || StampMesh == nullptr || StampMaterial == nullptr)
	{
		return;
	}
	
	// 공중이면 스탬프 X
	if (MoveComp->GetMovementMode() != ESkullyMovementMode::Grounded)
	{
		bHasLastSample = false;
		DistanceAccumCm = 0.0f;
		return;
	}
	
	if (Speed2D < MinSpeedToStamp)
	{
		bHasLastSample = false;
		DistanceAccumCm = 0.0f;
		return;
	}
	
	AActor* Owner = GetOwner();
	const FVector CurPos = Owner->GetActorLocation();
	
	if (bHasLastSample == false)
	{
		LastSamplePos = CurPos;
		bHasLastSample = true;
		DistanceAccumCm = 0.0f;
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[Trail] Speed=%.2f Mode=%d HasLast=%d"), Speed2D, (int)MoveComp->GetMovementMode(), bHasLastSample);
	
	const FVector PrevPos = LastSamplePos;
	LastSamplePos = CurPos;
	
	// 이번 프레임 이동거리(2D)
	const float DistMoved = FVector::Dist2D(CurPos, PrevPos);
	if (DistMoved <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	// --- 연속 트레일: 보간 스탬프 ---
	// DistanceAccumCm: 이전 프레임에서 남은 잔여 거리(스텝 미만)
	// PrevPos -> CurPos 사이에서 SampleSpacingCm마다 찍기
	const float Total = DistanceAccumCm + DistMoved;
	if (Total < SampleSpacingCm)
	{
		DistanceAccumCm = Total;
		return;
	}

	const int32 NumSteps = FMath::FloorToInt(Total / SampleSpacingCm);

	// PrevPos에서 얼마만큼 진행한 지점부터 첫 스탬프를 찍을지
	float FirstTravel = SampleSpacingCm - DistanceAccumCm;

	// 안전: Dir2D가 이상하면 Prev->Cur로 대체
	FVector MoveDir = FVector(Dir2D.X, Dir2D.Y, 0.f);
	if (MoveDir.IsNearlyZero() == true)
	{
		MoveDir = CurPos - PrevPos;
		MoveDir.Z = 0.0f;
	}
	MoveDir = MoveDir.GetSafeNormal();

	for (int32 i = 0; i < NumSteps; ++i)
	{
		const float Travel = FirstTravel + i * SampleSpacingCm;
		const float T = FMath::Clamp(Travel / DistMoved, 0.f, 1.f);

		FVector SamplePos = FMath::Lerp(PrevPos, CurPos, T);
		(void)TryStampAt(SamplePos, MoveDir);
	}

	// 남은 잔여 거리 저장
	DistanceAccumCm = Total - NumSteps * SampleSpacingCm;
}

bool USkullyTrailComponent::TryStampAt(const FVector& SampleWorldPos, const FVector& MoveDir2D)
{
	FHitResult Hit;
	
	const bool bHit = TraceGroundAt(SampleWorldPos, Hit);
	//UE_LOG(LogTemp, Warning, TEXT("[Trail] Trace=%d Blocking=%d Actor=%s PhysMat=%s"),
	//	bHit, Hit.bBlockingHit, *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.PhysMaterial.Get()));

	if (!bHit || !Hit.bBlockingHit) return false;
	
	UE_LOG(LogTemp, Warning, TEXT("[Trail] Stamp! Pos=%s"), *Hit.ImpactPoint.ToString());

	const FVector N = Hit.ImpactNormal.GetSafeNormal();
	const FVector Pos = Hit.ImpactPoint + N * SurfaceOffsetCm;

	FRotator Rot = UKismetMathLibrary::MakeRotFromZ(N);

	if (bAlignToMovementDir == true)
	{
		FVector Fwd = FVector(MoveDir2D.X, MoveDir2D.Y, 0.0f).GetSafeNormal();
		Fwd = FVector::VectorPlaneProject(Fwd, N).GetSafeNormal();

		if (!Fwd.IsNearlyZero())
		{
			Rot = UKismetMathLibrary::MakeRotFromXZ(Fwd, N);
		}
	}

	// 스케일 (엔진 Plane 100cm 가정)
	const float WidthScale  = TrailWidthCm / 100.0f;
	const float LengthScale = StampLengthCm / 100.0f;

	const FVector Scale(LengthScale, WidthScale, 1.0f);
	const FTransform Xf(Rot, Pos, Scale);

	const float SpawnTime = GetWorld()->GetTimeSeconds();

	// (선택) 속도 기반 강도: 빠르면 조금 더 진하게
	const float Strength = 1.0f;

	PlaceStampInstance(Xf, SpawnTime, Strength);
	return true;
}

bool USkullyTrailComponent::TraceGroundAt(const FVector& SampleWorldPos, FHitResult& OutHit) const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return false;
	}

	const FVector Start = SampleWorldPos + FVector(0.0f, 0.0f, TraceUpCm);
	const FVector End   = SampleWorldPos - FVector(0.0f, 0.0f, TraceDownCm);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkullyTrailTrace), false);
	Params.AddIgnoredActor(Owner);

	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel, Params);
}

void USkullyTrailComponent::PlaceStampInstance(const FTransform& WorldXform, float SpawnTimeSeconds, float Strength)
{
	if (StampActor == nullptr)
	{
		return;
	}
	
	StampActor->AddOrUpdateStamp(WorldXform, SpawnTimeSeconds, Strength);
}
