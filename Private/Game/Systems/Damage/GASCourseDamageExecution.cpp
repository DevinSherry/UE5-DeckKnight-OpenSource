// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/GASCourseDamageExecution.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Healing/GASC_HealingGameplayEffect.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/Systems/Damage/Statics/GASC_DamagePipelineStatics.h"
#include "Game/Systems/Healing/GASCourseHealingExecution.h"

struct GASCourseDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageMultiplier);

	GASCourseDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, IncomingDamage, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, CriticalChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, CriticalDamageMultiplier, Source, false);
	}
};

static const GASCourseDamageStatics& DamageStatics()
{
	static GASCourseDamageStatics DStatics;
	return DStatics;
} 

UGASCourseDamageExecution::UGASCourseDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageMultiplierDef);
}

void UGASCourseDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UGASCourseAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UGASCourseAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UGASCourseAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	FGameplayEffectSpec* Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
	
	// Gather the tags from the source and target as that can affect which buffs should be used
	const FGameplayTagContainer* SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	// ==========================
	// 0. Instantiate Debug log entry
	// ==========================
	
	FGameplayEffectContextHandle ContextHandle = ExecutionParams.GetOwningSpec().GetEffectContext();
	FGASCourseGameplayEffectContext* GASCourseContext = (FGASCourseGameplayEffectContext*)ContextHandle.Get();
	GASCourseContext->DamageLogEntry.HitInstigatorName = SourceActor->GetName();
	GASCourseContext->DamageLogEntry.HitTargetName = TargetActor->GetName();
	GASCourseContext->DamageLogEntry.HitInstigatorTagsContainer.AppendTags(*SourceTags);
	GASCourseContext->DamageLogEntry.HitTargetTagsContainer.AppendTags(*TargetTags);
	GASCourseContext->DamageLogEntry.HitContextTagsContainer.AppendTags(Spec->DynamicGrantedTags);
	GASCourseContext->DamageLogEntry.DamageInstigatorID = SourceActor->GetUniqueID();
	GASCourseContext->DamageLogEntry.DamageTargetID = TargetActor->GetUniqueID();

	bool bUsingCachedDamage = false;
	bool bCriticalHit = false;
	float Damage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IncomingDamageDef, EvaluationParameters, Damage);
	// Add SetByCaller damage if it exists
	Damage += FMath::Max<float>(Spec->GetSetByCallerMagnitude(Data_IncomingDamage, false, -1.0f), 0.0f);
	
	GASCourseContext->DamageLogEntry.BaseDamageValue = Damage;

	if (Spec->DynamicGrantedTags.HasTagExact(Data_DamageOverTime))
	{
		float CachedDamage = 0.0f;
		if (Spec->SetByCallerTagMagnitudes.Find(Data_CachedDamage))
		{
			CachedDamage = Spec->GetSetByCallerMagnitude(Data_CachedDamage);
		}
		if (CachedDamage > 0.0f)
		{
			Damage = CachedDamage;
			// Set the Target's damage meta attribute
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
			bUsingCachedDamage = true;
		}
	}

	if (!bUsingCachedDamage)
	{
		/*
		* Critical Chance + Critical Damage
		*/
		float CriticalChance = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
	
		float CriticalDamageMultiplier = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageMultiplierDef, EvaluationParameters, CriticalDamageMultiplier);
	
		const float Roll = FMath::FRand();   // 0..1
		bCriticalHit = (Roll <= CriticalChance);
		if (bCriticalHit)
		{
			Damage += FMath::Floor(Damage * CriticalDamageMultiplier);
			GASCourseContext->DamageLogEntry.Attributes.Add(DamageStatics().CriticalChanceProperty->GetName(), 
			CriticalChance);
			GASCourseContext->DamageLogEntry.Attributes.Add(DamageStatics().CriticalDamageMultiplierProperty->GetName(), 
			CriticalDamageMultiplier);
			Spec->DynamicGrantedTags.AddTag(DamageType_Critical);
		}

		if (Damage > 0.f)
		{
			// Set the Target's damage meta attribute
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
		}

		//Store damage as cached damage
		Spec->SetSetByCallerMagnitude(Data_CachedDamage, Damage);
		
		GASCourseContext->DamageLogEntry.ModifiedDamageValue = Damage;
	}
	
	if (UWorld* World = SourceActor->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			//TODO Add damage log entry for lifesteal labeling
			World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateLambda([this, DamagePipelineSubsystem, SourceActor, Damage]()
			{
					FDamagePipelineContext HealContext;
					DamagePipelineSubsystem->ApplyHealToTarget(SourceActor, SourceActor, Damage, HealContext);
			}));
		}
	}
	
	/*
	 *This is where we will add more damage log information for resistances and armor calculations
	 */

	// Broadcast damages to Target ASC & SourceASC
	if (TargetAbilitySystemComponent && SourceAbilitySystemComponent)
	{
		FGameplayEventData DamageDealtPayload;
		DamageDealtPayload.Instigator = SourceAbilitySystemComponent->GetAvatarActor();
		DamageDealtPayload.Target = TargetAbilitySystemComponent->GetAvatarActor();
		DamageDealtPayload.EventMagnitude = Damage;
		DamageDealtPayload.ContextHandle = Spec->GetContext();
		DamageDealtPayload.InstigatorTags = Spec->DynamicGrantedTags;
		if (const FHitResult* HR = Spec->GetContext().GetHitResult())
		{
			DamageDealtPayload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(*HR);
		}

		if (bCriticalHit)
		{
			DamageDealtPayload.InstigatorTags.AddTag(DamageType_Critical);
		}
		
		if(TargetAbilitySystemComponent->HasMatchingGameplayTag(Status_Death))
		{
			return;
		}
		
		SourceAbilitySystemComponent->HandleGameplayEvent(Event_Gameplay_OnDamageDealt, &DamageDealtPayload);
		TargetAbilitySystemComponent->HandleGameplayEvent(Event_Gameplay_OnDamageDealt, &DamageDealtPayload);

		//TODO: Instead of sending event, pass in status effect tag into gameplay status table
		TargetAbilitySystemComponent->ApplyGameplayStatusEffect(TargetAbilitySystemComponent, SourceAbilitySystemComponent, Spec->DynamicGrantedTags);
	}
}
 