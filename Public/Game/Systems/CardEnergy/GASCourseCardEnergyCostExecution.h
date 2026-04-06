// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GASCourseCardEnergyCostExecution.generated.h"

/**
 * UGASCourseCardEnergyCostExecution is a subclass of UGameplayEffectExecutionCalculation
 * designed to calculate and apply the energy cost of a gameplay effect. This execution is
 * invoked when a gameplay effect with this execution class is applied in the Ability System.
 */
UCLASS()
class GASCOURSE_API UGASCourseCardEnergyCostExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	
	UGASCourseCardEnergyCostExecution();
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
