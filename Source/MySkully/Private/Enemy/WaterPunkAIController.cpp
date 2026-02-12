#include "Enemy/WaterPunkAIController.h"

#include "BrainComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/WaterPunk.h"

AWaterPunkAIController::AWaterPunkAIController()
{
	// 감지 컴포넌트
	WaterPunkPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("WaterPunkPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1800.0f;
	SightConfig->LoseSightRadius = 2500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	WaterPunkPerception->ConfigureSense(*SightConfig);
	WaterPunkPerception->SetDominantSense(UAISense_Sight::StaticClass());
	WaterPunkPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AWaterPunkAIController::OnTargetPerceptionUpdated);
}

void AWaterPunkAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	bAIStarted = false;
}

void AWaterPunkAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (BB == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController.cpp][OnTargetPerceptionUpdated] BlackboardComponent = nullptr"));
		return;
	}
	
	AWaterPunk* WaterPunk = Cast<AWaterPunk>(GetPawn());
	if (WaterPunk == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController.cpp][OnTargetPerceptionUpdated] WaterPunk = nullptr"));
		return;
	}
	
	if (Stimulus.WasSuccessfullySensed() == true)
	{
		WaterPunk->SeePlayer();
		BB->SetValueAsObject(FName("TargetActor"), Actor);
	}
	else
	{
		WaterPunk->LostPlayer();
		BB->ClearValue(FName("TargetActor"));
	}
}

void AWaterPunkAIController::StartWaterPunkAI()
{
	if (bAIStarted == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][StartWaterPunkAI] bAIStarted = true"));
		return;
	}
	if (WaterPunkBehaviorTree == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][StartWaterPunkAI] WaterPunkBehaviorTree = nullptr"));
		return;
	}
	
	const bool bAIRun = RunBehaviorTree(WaterPunkBehaviorTree);
	if (bAIRun == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][StartWaterPunkAI] RunBehaviorTree is fail"));
		return;
	}
	
	AWaterPunk* WaterPunk = Cast<AWaterPunk>(GetPawn());
	if (WaterPunk == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][StartWaterPunkAI] WaterPunk = nullptr"));
		return;
	}
	WaterPunk->Explosion();
	
	bAIStarted = true;
}

void AWaterPunkAIController::StopWaterPunkAI()
{
	if (bAIStarted == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaterPunkAIController][StopWaterPunkAI] bAIStarted = false"))
		return;
	}
	
	bAIStarted = false;
	BrainComponent->StopLogic(TEXT("StopWaterPunkAI"));
}
