// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Healing/GASC_HealingOverTimeGameplayEffect.h"
#include "Game/Systems/Healing/GASCourseHealingExecution.h"

UGASC_HealingOverTimeGameplayEffect::UGASC_HealingOverTimeGameplayEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	bExecutePeriodicEffectOnApplication = true;
	DurationMagnitude = FScalableFloat(1.0f);

	// Attach the damage execution ONCE
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGASCourseHealingExecution::StaticClass();
	Executions.Add(ExecDef);
}