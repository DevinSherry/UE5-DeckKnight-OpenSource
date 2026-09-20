// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/WeaponMana/GASC_WeaponManaInstantGameplayEffect.h"
#include "Game/Systems/WeaponMana/GASCourseWeaponManaExecution.h"

UGASC_WeaponManaInstantGameplayEffect::UGASC_WeaponManaInstantGameplayEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Attach the damage execution ONCE
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGASCourseWeaponManaExecution::StaticClass();
	Executions.Add(ExecDef);
}
