// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Conditions/FGASCGameplayTagQueryCondition.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include "StateTreeExecutionContext.h"

#define LOCTEXT_NAMESPACE "StateTreeEditor"

namespace UE::StateTree::Conditions
{


}

bool FGASCGameplayTagQueryCondition::Link(FStateTreeLinker& Linker)
{
	return FStateTreeConditionCommonBase::Link(Linker);
}

FGameplayTagContainer FGASCGameplayTagQueryCondition::GetTagContainer(const FInstanceDataType& InstanceData) const
{
	if (InstanceData.bUseAlternateTagContainer)
	{
		return InstanceData.AlternativeTagContainer;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstanceData.ActorToCheck);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("FGASCGameplayTagQueryCondition: No AbilitySystemComponent found on ActorToCheck."));
		return FGameplayTagContainer();
	}
	return ASC->GetOwnedGameplayTags();
}

bool FGASCGameplayTagQueryCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const FGameplayTagContainer TagContainer = GetTagContainer(InstanceData);
	if (TagContainer.IsEmpty() && !InstanceData.bUseAlternateTagContainer)
	{
		return false;
	}

	const bool bResult = InstanceData.TagQuery.Matches(TagContainer) ^ bInvert;

	SET_NODE_CUSTOM_TRACE_TEXT(Context, Override, TEXT("%s'%s' matches %s")
		, *UE::StateTree::DescHelpers::GetInvertText(bInvert, EStateTreeNodeFormatting::Text).ToString()
		, *UE::StateTree::DescHelpers::GetGameplayTagContainerAsText(TagContainer).ToString()
		, *UE::StateTree::DescHelpers::GetGameplayTagQueryAsText(InstanceData.TagQuery).ToString());

	return bResult;
}

#if WITH_EDITOR

FText FGASCGameplayTagQueryCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText ContainerValue = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, AlternativeTagContainer)), Formatting);
	if (ContainerValue.IsEmpty())
	{
		ContainerValue = UE::StateTree::DescHelpers::GetGameplayTagContainerAsText(InstanceData->AlternativeTagContainer);
	}

	const FText QueryValue = UE::StateTree::DescHelpers::GetGameplayTagQueryAsText(InstanceData->TagQuery);

	const FText InvertText = UE::StateTree::DescHelpers::GetInvertText(bInvert, Formatting);

	FText Format;
	if (InstanceData->bUseAlternateTagContainer)
	{
		Format = (Formatting == EStateTreeNodeFormatting::RichText)
		? LOCTEXT("GameplayTagQueryRich", "{EmptyOrNot}{TagContainer} <s>matches</> {TagQuery}")
		: LOCTEXT("GameplayTagQuery", "{EmptyOrNot}{TagContainer} matches {TagQuery}");
	}
	else
	{
		Format = (Formatting == EStateTreeNodeFormatting::RichText)
		? LOCTEXT("GameplayTagQueryRich", "{EmptyOrNot}Actor Owning Gameplay Tags <s>matches</> {TagQuery}")
		: LOCTEXT("GameplayTagQuery", "{EmptyOrNot}Actor Owning Gameplay Tags matches {TagQuery}");
	}


	return FText::FormatNamed(Format,
		TEXT("EmptyOrNot"), InvertText,
		TEXT("TagContainer"), ContainerValue,
		TEXT("TagQuery"), QueryValue);
}

EDataValidationResult FGASCGameplayTagQueryCondition::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);
	
	if (InstanceData->bUseAlternateTagContainer && InstanceData->AlternativeTagContainer.IsEmpty())
	{
		CompileContext.AddValidationError(LOCTEXT("AlternativeTagContainerEmpty", "Alternative Tag Container is empty but bUseAlternateTagContainer is enabled."));
		return EDataValidationResult::Invalid;
	}
	const bool bIsActorBound = CompileContext.HasBindingForProperty(GET_MEMBER_NAME_CHECKED(FGASCGameplayTagQueryConditionInstanceData, ActorToCheck));
	if (!InstanceData->bUseAlternateTagContainer && !bIsActorBound)
	{
		CompileContext.AddValidationError(LOCTEXT("ActorToCheckNull", "ActorToCheck is null but bUseAlternateTagContainer is disabled."));
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
