// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/CardEnergy/GASCourseCardEnergyCostExecution.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASC_CardResourcesAttributeSet.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"
#include "Game/Character/Player/GASCoursePlayerState.h"
#include "GameFramework/Character.h"

struct GASCourseCardEnergyCostStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingCardEnergyCost);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CardEnergyCostMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CardEnergyCostOverride);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CardEnergyCostAdditive);
	
	GASCourseCardEnergyCostStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, IncomingCardEnergyCost, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, CardEnergyCostMultiplier, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, CardEnergyCostOverride, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, CardEnergyCostAdditive, Target, false);
	}
};

static const GASCourseCardEnergyCostStatics& CardEnergyCostStatics()
{
	static GASCourseCardEnergyCostStatics DStatics;
	return DStatics;
}

UGASCourseCardEnergyCostExecution::UGASCourseCardEnergyCostExecution()
{
	RelevantAttributesToCapture.Add(CardEnergyCostStatics().IncomingCardEnergyCostDef);
	RelevantAttributesToCapture.Add(CardEnergyCostStatics().CardEnergyCostMultiplierDef);
	RelevantAttributesToCapture.Add(CardEnergyCostStatics().CardEnergyCostOverrideDef);
	RelevantAttributesToCapture.Add(CardEnergyCostStatics().CardEnergyCostAdditiveDef);
}

void UGASCourseCardEnergyCostExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UGASCourseAbilitySystemComponent* TargetAbilitySystemComponent =
	Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UGASCourseAbilitySystemComponent* SourceAbilitySystemComponent =
		Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	FGameplayEffectSpec* Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();

	if (!SourceActor || !TargetActor || !Spec)
	{
		return;
	}
	
	// Gather tags
	const FGameplayTagContainer* SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	
	float CurrentCardCostOverride = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CardEnergyCostStatics().CardEnergyCostOverrideDef, EvaluationParameters, CurrentCardCostOverride);
	
	if (TargetAbilitySystemComponent->HasMatchingGameplayTag(Status_CardEnergyCostOverride))
	{
		OutExecutionOutput.AddOutputModifier(
	FGameplayModifierEvaluatedData(
		CardEnergyCostStatics().IncomingCardEnergyCostProperty,
		EGameplayModOp::Override,
		CurrentCardCostOverride));
	}
	
	float BaseCardCost = Spec->GetSetByCallerMagnitude(Data_CardCost, false, 0.f);
	
	float CardCostMultiplier = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CardEnergyCostStatics().CardEnergyCostMultiplierDef, EvaluationParameters, CardCostMultiplier);
	
	float CardCostAdditive = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CardEnergyCostStatics().CardEnergyCostAdditiveDef, EvaluationParameters, CardCostAdditive);
	
	bool bHasDebugIgnoreCost = false;
	if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(Cast<ACharacter>(SourceActor)->GetPlayerState()))
	{
		bHasDebugIgnoreCost = PS->GetDeckManagerComponent()->DebugIgnoreCardCostEnabled();
	}
	
	float ModifiedCardCost = bHasDebugIgnoreCost ? 0.0f : (BaseCardCost + CardCostAdditive) * CardCostMultiplier;
	OutExecutionOutput.AddOutputModifier(
	FGameplayModifierEvaluatedData(
		CardEnergyCostStatics().IncomingCardEnergyCostProperty,
		EGameplayModOp::Override,
		ModifiedCardCost));
}
