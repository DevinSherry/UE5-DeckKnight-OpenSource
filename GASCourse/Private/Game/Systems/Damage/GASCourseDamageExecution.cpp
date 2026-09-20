// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/GASCourseDamageExecution.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/WeaponMana/GASC_WeaponManaInstantGameplayEffect.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineSubsystem.h"
#include "Game/Systems/Healing/GASCourseHealingExecution.h"

struct GASCourseDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageResistanceMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageMultiplier);

	GASCourseDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, IncomingDamage, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, CriticalChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, CriticalDamageMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, DamageMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGASCourseHealthAttributeSet, DamageResistanceMultiplier, Target, false);
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
	RelevantAttributesToCapture.Add(DamageStatics().DamageMultiplierDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageResistanceMultiplierDef);
}

void UGASCourseDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
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

	if (TargetAbilitySystemComponent->HasMatchingGameplayTag(Status_Death))
	{
		return;
	}

	// Gather tags
	const FGameplayTagContainer* SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// ==========================
	// 0. Context & logging
	// ==========================

	FGameplayEffectContextHandle ContextHandle = Spec->GetEffectContext();
	FGameplayEffectContext* BaseCtx = ContextHandle.Get();
	if (!BaseCtx)
	{
		return;
	}

	auto* GASCourseContext = static_cast<FGASCourseGameplayEffectContext*>(BaseCtx);

	GASCourseContext->ResourceModLogEntry.HitInstigatorName = SourceActor->GetName();
	GASCourseContext->ResourceModLogEntry.HitTargetName = TargetActor->GetName();
	GASCourseContext->ResourceModLogEntry.HitInstigatorTagsContainer.AppendTags(Spec->CapturedSourceTags.GetActorTags());
	GASCourseContext->ResourceModLogEntry.HitTargetTagsContainer.AppendTags(Spec->CapturedTargetTags.GetActorTags());
	GASCourseContext->ResourceModLogEntry.HitContextTagsContainer.AppendTags(Spec->GetDynamicAssetTags());
	GASCourseContext->ResourceModLogEntry.ResourceInstigatorID = SourceActor->GetUniqueID();
	GASCourseContext->ResourceModLogEntry.ResourceTargetID = TargetActor->GetUniqueID();

	// ==========================
	// 1. Base damage
	// ==========================

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().IncomingDamageDef, EvaluationParameters, BaseDamage);

	BaseDamage += FMath::Max(
		Spec->GetSetByCallerMagnitude(Data_IncomingDamage, false, -1.0f),
		0.0f);

	float ModifiedDamage = BaseDamage;
	GASCourseContext->ResourceModLogEntry.BaseResourceValue = BaseDamage;

	// ==========================
	// 2. Snapshot DoT handling
	// ==========================

	const bool bIsDoT = Spec->GetDynamicAssetTags().HasTagExact(Data_DamageOverTime);
	bool bSkipDamageRecalculation = false;

	if (bIsDoT)
	{
		const float CachedDamage =
			Spec->GetSetByCallerMagnitude(Data_CachedDamage, false, 0.f);

		if (CachedDamage > 0.f)
		{
			ModifiedDamage = CachedDamage;

			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(
					DamageStatics().IncomingDamageProperty,
					EGameplayModOp::Additive,
					ModifiedDamage));

			bSkipDamageRecalculation = true;
		}
	}

	// ==========================
	// 3. Crit / multipliers / resistance (first application only)
	// ==========================

	if (!bSkipDamageRecalculation)
	{
		//If this is self inflicted damage, do not roll for crits
		if (TargetActor != SourceActor)
		{
			float CriticalChance = 0.0f;
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
			CriticalChance = FMath::Clamp(CriticalChance, 0.f, 1.f);

			float CriticalDamageMultiplier = 0.0f;
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics().CriticalDamageMultiplierDef, EvaluationParameters, CriticalDamageMultiplier);

			if (FMath::FRand() <= CriticalChance)
			{
				ModifiedDamage *= (1.f + CriticalDamageMultiplier);
				Spec->AddDynamicAssetTag(Data_DamageCritical);

				GASCourseContext->ResourceModLogEntry.Attributes.Add(
					GASCourseDamageStatics().CriticalChanceProperty->GetName(), CriticalChance);
				GASCourseContext->ResourceModLogEntry.Attributes.Add(
					GASCourseDamageStatics().CriticalDamageMultiplierProperty->GetName(), CriticalDamageMultiplier);
			}
		}

		float DamageMultiplier = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			GASCourseDamageStatics().DamageMultiplierDef, EvaluationParameters, DamageMultiplier);

		if (DamageMultiplier > 0.f)
		{
			ModifiedDamage += (ModifiedDamage * DamageMultiplier);
			GASCourseContext->ResourceModLogEntry.Attributes.Add(
				DamageStatics().DamageMultiplierProperty->GetName(), DamageMultiplier);
		}

		float DamageResistance = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			GASCourseDamageStatics().DamageResistanceMultiplierDef, EvaluationParameters, DamageResistance);
		DamageResistance = FMath::Clamp(DamageResistance, 0.f, 1.f);

		if (DamageResistance > 0.f)
		{
			ModifiedDamage *= (1.f - DamageResistance);
			Spec->AddDynamicAssetTag(Data_DamageResisted);
			GASCourseContext->ResourceModLogEntry.Attributes.Add(
				DamageStatics().DamageResistanceMultiplierProperty->GetName(), DamageResistance);
		}

		ModifiedDamage = FMath::Max(ModifiedDamage, 0.f);

		bool bIsPlayerSource = false;
		if (APawn* PawnSource = Cast<APawn>(SourceActor))
		{
			if (AController* Controller = PawnSource->GetController())
			{
				bIsPlayerSource = Controller->IsPlayerController();
			}
		}

		// Always round — choose method based on source
		ModifiedDamage = bIsPlayerSource
			? FMath::CeilToFloat(ModifiedDamage)
			: FMath::FloorToFloat(ModifiedDamage);

		if (ModifiedDamage > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(
					DamageStatics().IncomingDamageProperty,
					EGameplayModOp::Additive,
					ModifiedDamage));
		}

		// Cache snapshot damage for DoT ticks
		Spec->SetSetByCallerMagnitude(Data_CachedDamage, ModifiedDamage);

		GASCourseContext->ResourceModLogEntry.ModifiedResourceValue =
			ModifiedDamage - BaseDamage;
	}

	if (UWorld* World = SourceActor->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem =
			World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			if (ModifiedDamage > 0.f)
			{
				// ==========================
				// 4. Lifesteal
				// ==========================
				World->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateLambda(
						[this, DamagePipelineSubsystem, SourceActor, ModifiedDamage]()
						{
							FDamagePipelineContext HealContext;
							HealContext.GrantedTags.AddTag(Data_HealingLifeSteal);
							DamagePipelineSubsystem->ApplyHealToTarget(
								SourceActor, SourceActor, ModifiedDamage, HealContext);
						}));
			}
		}
	}
	
	

	// ==========================
	// 5. Gameplay events & status
	// ==========================

	FGameplayEventData DamageEvent;
	DamageEvent.Instigator = SourceActor;
	DamageEvent.Target = TargetActor;
	DamageEvent.EventMagnitude = ModifiedDamage;
	DamageEvent.ContextHandle = Spec->GetContext();
	DamageEvent.InstigatorTags = Spec->DynamicGrantedTags;

	if (const FHitResult* HR = Spec->GetContext().GetHitResult())
	{
		DamageEvent.TargetData =
			UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(*HR);
	}

	if (Spec->GetDynamicAssetTags().HasTagExact(Data_DamageCritical))
	{
		DamageEvent.InstigatorTags.AddTag(Data_DamageCritical);
	}

	SourceAbilitySystemComponent->HandleGameplayEvent(
		Event_Gameplay_OnDamageDealt, &DamageEvent);

	TargetAbilitySystemComponent->HandleGameplayEvent(
		Event_Gameplay_OnDamageReceived, &DamageEvent);

	TargetAbilitySystemComponent->ApplyGameplayStatusEffect(
		TargetAbilitySystemComponent,
		SourceAbilitySystemComponent,
		Spec->DynamicGrantedTags);
}


 