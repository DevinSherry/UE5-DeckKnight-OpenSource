#include "Game/Systems/Targeting/Conditional/GASC_TargetingPreset_BlockingExample.h"
#include "Game/Systems/Targeting/Conditional/GASC_TargetingCondition_MatchTagQuery.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_TargetingSelectionTask_AOE.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortDistance.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortInputAngle.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(GASC_TargetingPreset_BlockingExample)
UGASC_TargetingPreset_BlockingExample::UGASC_TargetingPreset_BlockingExample(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 auto* Gather = ObjectInitializer.CreateDefaultSubobject<UGASC_TargetingSelectionTask_AOE>(this, TEXT("Gather nearby pawns"));
 Gather->SetShapeType(ETargetingAOEShape::Sphere); Gather->SetRadius(1000.f); Gather->SetCollisionCollisionChannel(ECC_Pawn);
 Gather->SetIgnoreSourceActor(true); Gather->SetIgnoreInstigatorActor(true); Gather->AddCollisionObjectTypes(UEngineTypes::ConvertToObjectType(ECC_Pawn));
 TargetingTaskSet.Tasks.Add(Gather);
 auto* Condition = ObjectInitializer.CreateDefaultSubobject<UGASC_TargetingCondition_MatchTagQuery>(this, TEXT("While blocking - run Children"));
 Condition->Label = TEXT("While blocking: defensive targeting Children");
 Condition->TagSource = EGASCTargetingConditionTagSource::SourceActor;
 const FGameplayTag Blocking = FGameplayTag::RequestGameplayTag(TEXT("Status.Gameplay.Stance.Blocking"), false);
 if (Blocking.IsValid()) { FGameplayTagQueryExpression Expr; Expr.AllTagsMatch().AddTag(Blocking); Condition->MatchTagQuery = FGameplayTagQuery::BuildQuery(Expr); }
 auto* Distance = ObjectInitializer.CreateDefaultSubobject<UGASCourse_TargetSortDistance>(Condition, TEXT("Child 1 - prefer nearby threats"));
 Distance->MaxDistance = 1000.f; Distance->DefaultScoreMultiplier = 1.f;
 auto* Input = ObjectInitializer.CreateDefaultSubobject<UGASCourse_TargetSortInputAngle>(Condition, TEXT("Child 2 - prefer input direction"));
 Input->DefaultScoreMultiplier = .5f;
 Condition->Children.Add(Distance); Condition->Children.Add(Input);
 TargetingTaskSet.Tasks.Add(Condition);
}
