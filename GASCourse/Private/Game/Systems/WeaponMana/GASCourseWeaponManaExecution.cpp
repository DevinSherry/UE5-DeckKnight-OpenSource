// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/WeaponMana/GASCourseWeaponManaExecution.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASC_WeaponAttributeSet.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"

struct FGASCourseWeaponManaStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingWeaponMana);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingWeaponManaCritMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingWeaponManaKillMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(OutgoingWeaponManaMultiplier);
	
	FGASCourseWeaponManaStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_WeaponAttributeSet, IncomingWeaponMana, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_WeaponAttributeSet, IncomingWeaponManaCritMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_WeaponAttributeSet, IncomingWeaponManaKillMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_WeaponAttributeSet, OutgoingWeaponManaMultiplier, Target, false);
	}
};

static const FGASCourseWeaponManaStatics WeaponManaStatics()
{
	static FGASCourseWeaponManaStatics WMStatics;
	return WMStatics;
}

UGASCourseWeaponManaExecution::UGASCourseWeaponManaExecution()
{
	RelevantAttributesToCapture.Add(WeaponManaStatics().IncomingWeaponManaDef);
	RelevantAttributesToCapture.Add(WeaponManaStatics().IncomingWeaponManaCritMultiplierDef);
	RelevantAttributesToCapture.Add(WeaponManaStatics().IncomingWeaponManaKillMultiplierDef);
	RelevantAttributesToCapture.Add(WeaponManaStatics().OutgoingWeaponManaMultiplierDef);
}

void UGASCourseWeaponManaExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	
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
	
	//TODO Checks for 'silence' status 
	
	// Gather tags
	const FGameplayTagContainer* SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	const FGameplayTagContainer& AssetTags = Spec->GetDynamicAssetTags();
	
	float BaseWeaponMana = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		WeaponManaStatics().IncomingWeaponManaDef, EvaluationParameters, BaseWeaponMana);
	
	float WeaponManaCriticalMultiplier = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		WeaponManaStatics().IncomingWeaponManaCritMultiplierDef, EvaluationParameters, WeaponManaCriticalMultiplier);
	
	float WeaponManaKillMultiplier = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		WeaponManaStatics().IncomingWeaponManaKillMultiplierDef, EvaluationParameters, WeaponManaKillMultiplier);
	
	float OutgoingWeaponManaMultiplier = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		WeaponManaStatics().OutgoingWeaponManaMultiplierDef, EvaluationParameters, OutgoingWeaponManaMultiplier);

	BaseWeaponMana += FMath::Max(
		Spec->GetSetByCallerMagnitude(Data_IncomingWeaponMana, false, -1.0f),
		0.0f);

	float ModifiedWeaponMana = BaseWeaponMana;
	ModifiedWeaponMana += (ModifiedWeaponMana * OutgoingWeaponManaMultiplier);
	
	if (AssetTags.HasTagExact(Data_DamageCritical))
	{
		ModifiedWeaponMana += (ModifiedWeaponMana * WeaponManaCriticalMultiplier);
	}
	
	if (AssetTags.HasTagExact(Data_DamageKilled))
	{
		ModifiedWeaponMana += (ModifiedWeaponMana * WeaponManaKillMultiplier);
	}
	
	if (ModifiedWeaponMana > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				WeaponManaStatics().IncomingWeaponManaProperty,
				EGameplayModOp::Additive,
				ModifiedWeaponMana));
	}
}
