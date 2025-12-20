// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Healing/GASCourseHealingExecution.h"
#include "Game/Character/NPC/GASCourseNPC_Base.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"

struct GASCourseHealingStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingHealing);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ElementalDamageHealingCoefficient)
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDamageHealingCoefficient)
	DECLARE_ATTRIBUTE_CAPTUREDEF(AllDamageHealingCoefficient)

	GASCourseHealingStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, IncomingHealing, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, ElementalDamageHealingCoefficient, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, PhysicalDamageHealingCoefficient, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, AllDamageHealingCoefficient, Target, false);
	}
};

static const GASCourseHealingStatics& HealingStatics()
{
	static GASCourseHealingStatics DStatics;
	return DStatics;
} 

UGASCourseHealingExecution::UGASCourseHealingExecution()
{
	RelevantAttributesToCapture.Add(HealingStatics().ElementalDamageHealingCoefficientDef);
	RelevantAttributesToCapture.Add(HealingStatics().PhysicalDamageHealingCoefficientDef);
	RelevantAttributesToCapture.Add(HealingStatics().AllDamageHealingCoefficientDef);
}

void UGASCourseHealingExecution::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UGASCourseAbilitySystemComponent* TargetASC =
		Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	const UGASCourseAbilitySystemComponent* SourceASC =
		Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Gather tags once
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;
	
	// ==========================
	// 0. Instantiate Debug log entry
	// ==========================
	
	FGameplayEffectContextHandle ContextHandle = ExecutionParams.GetOwningSpec().GetEffectContext();
	FGASCourseGameplayEffectContext* GASCourseContext = (FGASCourseGameplayEffectContext*)ContextHandle.Get();
	GASCourseContext->DamageLogEntry.HitInstigatorName = SourceActor->GetName();
	GASCourseContext->DamageLogEntry.HitTargetName = TargetActor->GetName();
	GASCourseContext->DamageLogEntry.HitInstigatorTagsContainer.AppendTags(*SourceTags);
	GASCourseContext->DamageLogEntry.HitTargetTagsContainer.AppendTags(*TargetTags);
	GASCourseContext->DamageLogEntry.HitContextTagsContainer.AppendTags(Spec.DynamicGrantedTags);
	GASCourseContext->DamageLogEntry.DamageInstigatorID = SourceActor->GetUniqueID();
	GASCourseContext->DamageLogEntry.DamageTargetID = TargetActor->GetUniqueID();

	// ==========================
	// 1. BASE HEAL AMOUNT
	// ==========================

	// SetByCaller is usually your primary knob -> add it in
	const float Healing = FMath::Max(
		Spec.GetSetByCallerMagnitude(Data_IncomingHealing, true, -1.0f),
		0.0f);

	// If still zero or negative → hard early out, no need to calculate anything else
	GASCourseContext->DamageLogEntry.BaseDamageValue = Healing;
	if (Healing <= 0.f)
	{
		return;
	}

	// ==========================
	// 2. DAMAGE TYPE TAGS
	// ==========================
	// Instead of building a new container, use a reference to existing dynamic tags.
	const FGameplayTagContainer& DynamicTags = Spec.DynamicGrantedTags;
	
	const bool bHasPhysical  = DynamicTags.HasTag(DamageType_Physical);
	const bool bHasElemental = DynamicTags.HasTag(DamageType_Elemental);

	// If no specific damage types, we only need AllHealingCoefficient.
	const bool bHasSpecificType =
		bHasPhysical || bHasElemental;

	// ==========================
	// 3. CAPTURE ONLY NEEDED COEFFICIENTS
	// ==========================

	float AllHealingCoefficient = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		HealingStatics().AllDamageHealingCoefficientDef, EvalParams, AllHealingCoefficient);
	GASCourseContext->DamageLogEntry.Attributes.Add(HealingStatics().AllDamageHealingCoefficientProperty->GetName(), 
	AllHealingCoefficient);
	
	float ElementalHealingCoefficient = 0.0f;
	float PhysicalHealingCoefficient  = 0.0f;

	if (bHasSpecificType)
	{
		if (bHasElemental)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				HealingStatics().ElementalDamageHealingCoefficientDef, EvalParams, ElementalHealingCoefficient);
			GASCourseContext->DamageLogEntry.Attributes.Add(HealingStatics().ElementalDamageHealingCoefficientProperty->GetName(), 
			ElementalHealingCoefficient);
		}

		if (bHasPhysical)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				HealingStatics().PhysicalDamageHealingCoefficientDef, EvalParams, PhysicalHealingCoefficient);
			GASCourseContext->DamageLogEntry.Attributes.Add(HealingStatics().PhysicalDamageHealingCoefficientProperty->GetName(), 
			PhysicalHealingCoefficient);
		}
	}

	// ==========================
	// 5. COEFFICIENT APPLICATION
	// ==========================
	float TotalHealing = 0.0f;

	if (!bHasSpecificType)
	{
		TotalHealing = Healing + (Healing * AllHealingCoefficient);
	}
	else
	{
		if (bHasPhysical)
		{
			TotalHealing += Healing * PhysicalHealingCoefficient;
		}
		if (bHasElemental)
		{
			TotalHealing += Healing * ElementalHealingCoefficient;
		}

		TotalHealing += Healing * AllHealingCoefficient;
	}

	if (TotalHealing <= 0.f)
	{
		return;
	}
	
	if (TargetActor!=SourceActor)
	{
		TotalHealing = Healing;
	}
	
	GASCourseContext->DamageLogEntry.ModifiedDamageValue = TotalHealing;

	// ==========================
	// 6. APPLY TO ATTRIBUTE
	// ==========================
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		HealingStatics().IncomingHealingProperty,
		EGameplayModOp::Additive,
		TotalHealing));
}