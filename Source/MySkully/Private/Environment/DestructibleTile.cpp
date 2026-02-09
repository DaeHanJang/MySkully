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
	
	// 블록 콜리전
	BlockCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockCollision"));
	BlockCollision->SetupAttachment(RootComponent);
	BlockCollision->SetRelativeLocation(FVector(280.0f, 0.0f, 310.0f));
	BlockCollision->InitBoxExtent(FVector(385.0f, 135.0f, 380.0f));
	BlockCollision->SetCollisionProfileName(TEXT("BlockAll"));
	BlockCollision->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	BlockCollision->PrimaryComponentTick.bCanEverTick = false;
	
	// 오버랩 콜리전
	OverlapCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapCollision"));
	OverlapCollision->SetupAttachment(RootComponent);
	OverlapCollision->SetRelativeLocation(FVector(280.0f, 0.0f, 310.0f));
	OverlapCollision->InitBoxExtent(FVector(385.0f, 300.0f, 380.0f));
	OverlapCollision->SetCollisionProfileName(TEXT("OverlapAll"));
	OverlapCollision->SetGenerateOverlapEvents(true);
	OverlapCollision->PrimaryComponentTick.bCanEverTick = false;
}

void ADestructibleTile::ApplyPunchAt(const FVector& WorldPos, const float Strain, const float VelocityMag)
{
	if (GeometryCollection == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DestructibleTile.cpp][ApplyPunchAt] GeometryCollection == nullptr"));
		return;
	}
	
	BlockCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	const TObjectPtr<UFieldSystemMetaDataFilter> MetaData = NewObject<UFieldSystemMetaDataFilter>();
	MetaData->SetMetaDataFilterType(EFieldFilterType::Field_Filter_All, EFieldObjectType::Field_Object_All, EFieldPositionType::Field_Position_CenterOfMass);

	UUniformScalar* UniformScalar = NewObject<UUniformScalar>();
	UniformScalar->Magnitude = Strain;
	GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain, MetaData.Get(), UniformScalar);

	URadialVector* RadialVel = NewObject<URadialVector>();
	RadialVel->Magnitude = VelocityMag;
	RadialVel->Position  = WorldPos;
	GeometryCollection->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, MetaData.Get(), RadialVel);
}
