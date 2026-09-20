// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "GASCourseCardEnergyExecution.generated.h"

/**
 * \class UGASCourseCardEnergyExecution
 * \brief Custom execution calculation responsible for handling specific gameplay effect logic.
 *
 * This class is a derived implementation of UGameplayEffectExecutionCalculation, designed to handle custom logic
 * for gameplay effects within the "GASCourse" system. It encapsulates the logic needed for calculating and applying
 * specific effects with a focus on energy-related behavior in the context of the gameplay ability system.
 */
UCLASS()
class GASCOURSE_API UGASCourseCardEnergyExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:

	UGASCourseCardEnergyExecution();

	/** \brief Executes the custom gameplay effect implementation.
	 *
	 * This method is invoked when a gameplay effect with a custom execution is applied. It calculates and applies the damage to the target actor,
	 * taking into account any damage modifiers and tags. It also broadcasts the damage dealt event to the target and source ability system components.
	 *
	 * \param ExecutionParams The parameters for the execution of the gameplay effect.
	 * \param OutExecutionOutput The output data of the execution of the gameplay effect.
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
	
	
};
