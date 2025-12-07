// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectTypes.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineTypes.h"
#include "GASC_GameplayEffectContextTypes.generated.h"

USTRUCT(BlueprintType)
struct FGASCourseGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	UPROPERTY()
	FDamageLogEntry DamageLogEntry;
	
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FGASCourseGameplayEffectContext* NewContext = new FGASCourseGameplayEffectContext(*this);
		return NewContext;
	}
};

