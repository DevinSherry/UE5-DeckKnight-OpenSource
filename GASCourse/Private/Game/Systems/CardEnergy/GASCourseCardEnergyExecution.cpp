// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/CardEnergy/GASCourseCardEnergyExecution.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASC_CardResourcesAttributeSet.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/Systems/CardEnergy/ActiveCardEnergy/GASC_ActiveCardResourceManager.h"
#include "GASCourse/GASCourseCharacter.h"

struct GASCourseCardEnergyXPStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingCardEnergyXP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentCardEnergy);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaximumCardEnergy);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ActiveCardEnergyMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(OnKillActiveCardEnergyMultiplier);

	GASCourseCardEnergyXPStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, IncomingCardEnergyXP, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, CurrentCardEnergy, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, MaximumCardEnergy, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, ActiveCardEnergyMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASC_CardResourcesAttributeSet, OnKillActiveCardEnergyMultiplier, Source, false);
	}
};

static const GASCourseCardEnergyXPStatics& CardEnergyXPStatics()
{
	static GASCourseCardEnergyXPStatics DStatics;
	return DStatics;
} 

UGASCourseCardEnergyExecution::UGASCourseCardEnergyExecution()
{
	RelevantAttributesToCapture.Add(CardEnergyXPStatics().IncomingCardEnergyXPDef);
	RelevantAttributesToCapture.Add(CardEnergyXPStatics().CurrentCardEnergyDef);
	RelevantAttributesToCapture.Add(CardEnergyXPStatics().MaximumCardEnergyDef);
	RelevantAttributesToCapture.Add(CardEnergyXPStatics().ActiveCardEnergyMultiplierDef);
	RelevantAttributesToCapture.Add(CardEnergyXPStatics().OnKillActiveCardEnergyMultiplierDef);
}

namespace
{
	constexpr float Zero = 0.0f;
	constexpr float DefaultMagnitude = -1.0f;

	inline UGASC_ActiveCardResourceManager* GetActiveCardResourceManagerFromActor(const AActor* SourceActor)
	{
		const AGASCourseCharacter* SourceCharacter = Cast<AGASCourseCharacter>(SourceActor);
		if (!SourceCharacter || !SourceCharacter->IsPlayerControlled())
		{
			return nullptr;
		}

		const APlayerController* PlayerController = Cast<APlayerController>(SourceCharacter->GetController());
		if (!PlayerController)
		{
			return nullptr;
		}

		const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		return LocalPlayer ? LocalPlayer->GetSubsystem<UGASC_ActiveCardResourceManager>() : nullptr;
	}

	inline void UpdateActiveCardEnergyXPDebug(const AActor* InSourceActor,
											  float InEnergyToAdd, const TMap<FGameplayAttribute, float>& Attributes)
	{
		UGASC_ActiveCardResourceManager* ResourceManager = GetActiveCardResourceManagerFromActor(InSourceActor);
		if (!ResourceManager)
		{
			return;
		}

		TArray<FActiveCardEnergyXPHistoryEntry>& HistoryArray = ResourceManager->GetActiveCardEnergyHistory();
		const int32 Count = HistoryArray.Num();
		if (Count <= 0)
		{
			return;
		}

		const int32 LastIndex = Count - 1;
		FActiveCardEnergyXPHistoryEntry& LastEntry = HistoryArray[LastIndex];
		LastEntry.ActiveCardEnergyXPModifiedValue = InEnergyToAdd;
		
		FString ModificationToolTip = "";
		for (const TPair<FGameplayAttribute, float>& Attribute : Attributes)
		{
			if (Attribute.Value != 1.0f && Attribute.Key != nullptr)
			{
				ModificationToolTip.Append(LINE_TERMINATOR).Appendf(TEXT("Attribute: %s -> %f"), *Attribute.Key.AttributeName, Attribute.Value);
			}
		}
		LastEntry.ModificationToolTip = ModificationToolTip;
	}

	inline bool IsEligibleForEnergyGain(const FGameplayEffectCustomExecutionParameters& ExecParams,
	                                    const FAggregatorEvaluateParameters& EvalParams,
	                                    const FGameplayEffectSpec& Spec,
	                                    float& OutEnergyToAdd)
	{
		const auto& Statics = CardEnergyXPStatics();

		float Current = Zero;
		ExecParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CurrentCardEnergyDef, EvalParams, Current);

		float Max = Zero;
		ExecParams.AttemptCalculateCapturedAttributeMagnitude(Statics.MaximumCardEnergyDef, EvalParams, Max);

		if (Current > Max)
		{
			OutEnergyToAdd = Zero;
			return false;
		}

		// Base captured incoming XP
		ExecParams.AttemptCalculateCapturedAttributeMagnitude(Statics.IncomingCardEnergyXPDef, EvalParams, OutEnergyToAdd);

		// Optional SetByCaller additive
		if (Spec.SetByCallerTagMagnitudes.Find(Data_IncomingCardEnergyXP))
		{
			OutEnergyToAdd += FMath::Max<float>(Spec.GetSetByCallerMagnitude(Data_IncomingCardEnergyXP, true, DefaultMagnitude), Zero);
		}
		return true;
	}

	inline void ApplyActiveEnergyXPMultipliers(const FGameplayEffectCustomExecutionParameters& ExecParams,
	                                   const FAggregatorEvaluateParameters& EvalParams,
	                                   const FGameplayEffectSpec& Spec,
	                                   float& InOutEnergy)
	{
		const auto& Statics = CardEnergyXPStatics();

		UGASCourseAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecParams.GetTargetAbilitySystemComponent());
		UGASCourseAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecParams.GetSourceAbilitySystemComponent());
	
		AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
		AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;
		
		TMap<FGameplayAttribute, float> AttributeMagnitudes;

		if (!Spec.DynamicGrantedTags.HasTagExact(Data_ActiveCardEnergyXP))
		{
			return;
		}

		float ActiveMultiplier = 0.0f;
		ExecParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ActiveCardEnergyMultiplierDef, EvalParams, ActiveMultiplier);
		InOutEnergy *= ActiveMultiplier;
		if (ActiveMultiplier != 0.0f)
		{
			AttributeMagnitudes.Add(Statics.ActiveCardEnergyMultiplierProperty, ActiveMultiplier);
		}

		if (Spec.DynamicGrantedTags.HasTagExact(Event_OnDeathDealt))
		{
			float OnKillMultiplier = 0.0f;
			ExecParams.AttemptCalculateCapturedAttributeMagnitude(Statics.OnKillActiveCardEnergyMultiplierDef, EvalParams, OnKillMultiplier);
			InOutEnergy *= OnKillMultiplier;
			if (OnKillMultiplier != 0.0f)
			{
				AttributeMagnitudes.Add(Statics.OnKillActiveCardEnergyMultiplierProperty, OnKillMultiplier);
			}
		}

		UpdateActiveCardEnergyXPDebug(SourceActor, InOutEnergy, AttributeMagnitudes);
	}
} // namespace

void UGASCourseCardEnergyExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	UGASCourseAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UGASCourseAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());
	
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	float EnergyToAdd = Zero;
	if (!IsEligibleForEnergyGain(ExecutionParams, EvalParams, Spec, EnergyToAdd))
	{
		return;
	}

	ApplyActiveEnergyXPMultipliers(ExecutionParams, EvalParams, Spec, EnergyToAdd);
	
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(CardEnergyXPStatics().IncomingCardEnergyXPProperty, EGameplayModOp::Additive, EnergyToAdd));
}
