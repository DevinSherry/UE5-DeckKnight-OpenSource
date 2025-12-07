// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Damage/GASC_DamageGameplayEffect.h"
#include "Game/Systems/Damage/GASCourseDamageExecution.h"

UGASC_DamageGameplayEffect::UGASC_DamageGameplayEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Attach the damage execution ONCE
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGASCourseDamageExecution::StaticClass();
	Executions.Add(ExecDef);
}
