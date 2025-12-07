// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemGlobals.h"
#include "GASC_AbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_AbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()
	
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};