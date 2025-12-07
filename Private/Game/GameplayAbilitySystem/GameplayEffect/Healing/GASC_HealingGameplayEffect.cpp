// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Healing/GASC_HealingGameplayEffect.h"
#include "Game/Systems/Healing/GASCourseHealingExecution.h"

UGASC_HealingGameplayEffect::UGASC_HealingGameplayEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Attach the damage execution ONCE
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGASCourseHealingExecution::StaticClass();
	Executions.Add(ExecDef);
}