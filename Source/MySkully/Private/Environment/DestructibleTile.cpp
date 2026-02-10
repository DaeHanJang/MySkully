#include "Environment/DestructibleTile.h"

#include "Components/BoxComponent.h"
#include "Field/FieldSystemObjects.h"
#include "GeometryCollection/GeometryCollectionComponent.h"


ADestructibleTile::ADestructibleTile()
{
 	PrimaryActorTick.bCanEverTick = false;
	
	// 씬 컴포넌트(루트)
	SceneComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
	SceneComponent->PrimaryComponentTick.bCanEverTick = false;
	
	// 지오메트리 컬렉션
	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollection->SetupAttachment(RootComponent);
	GeometryCollection->SetCollisionProfileName(TEXT("Destructible"));
	GeometryCollection->SetNotifyBreaks(true);
	GeometryCollection->OnChaosBreakEvent.AddDynamic(this, &ADestructibleTile::OnBreak);
	
	// 블록 콜리전
	BlockCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockCollision"));
	BlockCollision->SetupAttachment(RootComponent);
	BlockCollision->SetRelativeLocation(FVector(280.0f, 0.0f, 310.0f));
	BlockCollision->InitBoxExtent(FVector(385.0f, 135.0f, 380.0f));
	BlockCollision->SetCollisionProfileName(TEXT("BlockAll"));
	BlockCollision->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	BlockCollision->PrimaryComponentTick.bCanEverTick = false;
}

void ADestructibleTile::ApplyPunchAt(const FVector& PunchDir, const FVector& WorldPos, const float Strain, const float VelocityMag)
{
	if (GeometryCollection == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DestructibleTile.cpp][ApplyPunchAt] GeometryCollection == nullptr"));
		return;
	}
	
	BlockCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	UFieldSystemMetaDataFilter* MetaData = NewObject<UFieldSystemMetaDataFilter>(this);
	MetaData->SetMetaDataFilterType(EFieldFilterType::Field_Filter_All, EFieldObjectType::Field_Object_All, EFieldPositionType::Field_Position_CenterOfMass);

	UUniformScalar* UniformScalar = NewObject<UUniformScalar>(this);
	UniformScalar->Magnitude = Strain;
	GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain, MetaData, UniformScalar);

	const FVector Dir = PunchDir.GetSafeNormal(); 
	if (Dir.IsNearlyZero() == false)
	{
		UUniformVector* PushVel = NewObject<UUniformVector>(this);
		PushVel->Direction = Dir;
		PushVel->Magnitude = VelocityMag;
		GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, MetaData, PushVel);
	}
	else
	{
		URadialVector* RadialVel = NewObject<URadialVector>(this);
		RadialVel->Position  = WorldPos == FVector::ZeroVector ? BlockCollision->GetComponentLocation() : WorldPos;
		RadialVel->Magnitude = VelocityMag;
		GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, MetaData, RadialVel);
	}
}

void ADestructibleTile::OnBreak(const FChaosBreakEvent& BreakingData)
{
	UE_LOG(LogTemp, Warning, TEXT("[DestructibleTile][OnBreak]"));
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ADestructibleTile::UpdateDestroyTransition, 0.01f, true, 5.0f);
}
void ADestructibleTile::UpdateDestroyTransition()
{
	if (DestroyElapsedTime == 0.0f)
	{
		UFieldSystemMetaDataFilter* MetaData = NewObject<UFieldSystemMetaDataFilter>(this);
		MetaData->SetMetaDataFilterType(EFieldFilterType::Field_Filter_All, EFieldObjectType::Field_Object_All, EFieldPositionType::Field_Position_CenterOfMass);

		UUniformInteger* MakeKinematic = NewObject<UUniformInteger>(this);
		MakeKinematic->Magnitude = (int32)Chaos::EObjectStateType::Kinematic;
		GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_DynamicState, MetaData, MakeKinematic);
		
		GeometryCollection->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		GeometryCollection->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}
	
	DestroyElapsedTime += 0.01f;
	
	if (DestroyElapsedTime >= 2.0f)
	{
		GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
		Destroy();
	}
	else
	{
		GeometryCollection->AddWorldOffset(FVector::DownVector * 1.0f, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
