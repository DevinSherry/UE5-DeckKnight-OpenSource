// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GASC_AbilitySystemGlobals.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"

FGameplayEffectContext* UGASC_AbilitySystemGlobals::AllocGameplayEffectContext() const
{
	FGASCourseGameplayEffectContext* Context = new FGASCourseGameplayEffectContext();
	return Context;
}
