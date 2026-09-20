#pragma once
#include "Tasks/TargetingTask_Conditional.h"
#include "GameplayTagContainer.h"
#include "GASC_TargetingCondition_MatchTagQuery.generated.h"
UENUM(BlueprintType)
enum class EGASCTargetingConditionTagSource : uint8 { Instigator, SourceActor };
/** Matches tags once on the request actor; does not filter individual targets. Empty queries do not run children. */
UCLASS(Blueprintable, EditInlineNew, DisplayName="Condition: Match Tag Query (runs Children)")
class GASCOURSE_API UGASC_TargetingCondition_MatchTagQuery : public UTargetingTask_Conditional
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition", meta=(DisplayName="Actor whose tags must match"))
 EGASCTargetingConditionTagSource TagSource = EGASCTargetingConditionTagSource::Instigator;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Condition", meta=(DisplayName="Match Tag Query"))
 FGameplayTagQuery MatchTagQuery;
#if WITH_EDITOR
 virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
 virtual bool EvaluateCondition_Implementation(const FTargetingRequestHandle& Handle, FString& Reason) const override;
};
