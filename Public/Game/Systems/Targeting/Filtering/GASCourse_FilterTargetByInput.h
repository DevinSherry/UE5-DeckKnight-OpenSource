// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Systems/Targeting/Filtering/GASCourse_TargetFilterBase.h"
#include "GASCourse_FilterTargetByInput.generated.h"

/**
 * Represents a class responsible for filtering target entities based on specified input criteria.
 * Provides functionality to determine which targets meet the necessary input conditions
 * as part of a gameplay ability system.
 */
UCLASS()
class GASCOURSE_API UGASCourse_FilterTargetByInput : public UGASCourse_TargetFilterBase
{
	GENERATED_BODY()
	
public:
	
	/** Represents the minimum threshold value for input validation to ensure data meets required criteria */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="GASCourse|Targeting|Filtering")
	float MinInputThreshold = 0.1;

	/** Indicates whether all targets should be filtered out when no input is provided, controlling behavior during input absence. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="GASCourse|Targeting|Filtering")
	bool bFilterAllTargetsWhenNoInput = false;

	/**
	 * Defines the minimum dot product angle threshold to determine whether the target is not filtered
	 */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="GASCourse|Targeting|Filtering")
	float MinInputDotAngle = 0.5f;

	/**
	 * A variable that determines whether the most recently targeted melee enemy
	 * should be retained. Used to maintain consistency in target tracking
	 * during melee combat scenarios, ensuring smoother player experience.
	 */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="GASCourse|Targeting|Filtering")
	bool PreserveRecentMeleeTarget = true;
           
protected:
	
	/** Called against every target data to determine if the target should be filtered out */
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const;
};
