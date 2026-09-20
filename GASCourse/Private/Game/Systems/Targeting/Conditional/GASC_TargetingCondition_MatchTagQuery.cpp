#include "Game/Systems/Targeting/Conditional/GASC_TargetingCondition_MatchTagQuery.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagAssetInterface.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(GASC_TargetingCondition_MatchTagQuery)
bool UGASC_TargetingCondition_MatchTagQuery::EvaluateCondition_Implementation(const FTargetingRequestHandle& Handle, FString& Reason) const
{
 const auto* Context = FTargetingSourceContext::Find(Handle);
 AActor* Actor = Context ? (TagSource == EGASCTargetingConditionTagSource::Instigator ? Context->InstigatorActor.Get() : Context->SourceActor.Get()) : nullptr;
 if (!Actor) { 
#if !UE_BUILD_SHIPPING
 Reason = TEXT("Configured tag-source actor is missing; children skipped.");
#endif
 return false; }
 if (MatchTagQuery.IsEmpty()) { 
#if !UE_BUILD_SHIPPING
 Reason = TEXT("Match Tag Query is empty; configure a query to enable children.");
#endif
 return false; }
 FGameplayTagContainer Tags;
 if (const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)) ASC->GetOwnedGameplayTags(Tags);
 else if (const auto* Interface = Cast<IGameplayTagAssetInterface>(Actor)) Interface->GetOwnedGameplayTags(Tags);
 else { 
#if !UE_BUILD_SHIPPING
 Reason = TEXT("Actor has no ability system or gameplay-tag interface; children skipped.");
#endif
 return false; }
 const bool Match = MatchTagQuery.Matches(Tags);
#if !UE_BUILD_SHIPPING
 Reason = FString::Printf(TEXT("Actor %s; query %s; owned tags [%s]; %s."), *Actor->GetName(), *MatchTagQuery.GetDescription(), *Tags.ToStringSimple(), Match ? TEXT("query matched") : TEXT("query did not match"));
#endif
 return Match;
}

#if WITH_EDITOR
#include "Misc/DataValidation.h"
EDataValidationResult UGASC_TargetingCondition_MatchTagQuery::IsDataValid(FDataValidationContext& Context) const
{
 const auto Result = Super::IsDataValid(Context);
 if (MatchTagQuery.IsEmpty())
 {
  Context.AddError(FText::FromString(TEXT("Configure Match Tag Query: an empty query always skips Children.")));
  return EDataValidationResult::Invalid;
 }
 return Result;
}
#endif
