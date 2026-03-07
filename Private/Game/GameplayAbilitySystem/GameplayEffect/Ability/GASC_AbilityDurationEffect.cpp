// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Ability/GASC_AbilityDurationEffect.h"

UGASC_AbilityDurationEffect::UGASC_AbilityDurationEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	bExecutePeriodicEffectOnApplication = false;
	//DurationMagnitude = FScalableFloat(1.0f);

	/*
	 * TODO: If we want to introduce an execution class to handle duration based modifiers.
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGASCourseHealingExecution::StaticClass();
	Executions.Add(ExecDef);
	*/
}