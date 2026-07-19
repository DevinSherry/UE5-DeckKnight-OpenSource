// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GASCourseWeaponManaExecution.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASCourseWeaponManaExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	
	UGASCourseWeaponManaExecution();
	
	virtual  void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
