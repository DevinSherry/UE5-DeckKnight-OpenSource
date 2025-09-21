// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Engine/DataAsset.h"
#include "ActiveCardResourceEventMappingData.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_ActiveCardResourceMapping, Log, All);

/**
 * UActiveCardResourceEventMappingData class is a data asset used to map gameplay events
 * to scalable float values using FGameplayTag as the key.
 */
UCLASS()
class GASCOURSE_API UActiveCardResourceEventMappingData : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * GameplayEventMapping is a TMap that associates FGameplayTag keys with FScalableFloat values, used to map
	 * specific gameplay events to scalable numerical values for further processing in gameplay logic.
	 */
	UPROPERTY(EditAnywhere, Category = "GameplayEventMapping")
	TMap<FGameplayTag, FScalableFloat> GameplayEventMapping;

	/**
	 * Retrieves the mapped value associated with a provided gameplay event tag and level.
	 * Searches the internal GameplayEventMapping for a valid entry corresponding to the
	 * given FGameplayTag and scale, and returns the computed value if found.
	 *
	 * @param GameplayEventTag The gameplay event tag used as a key to find the corresponding mapping.
	 * @param InLevelValue The level value used to scale the mapped value.
	 * @param bValueFound Output parameter set to true if a matching mapping was found, false otherwise.
	 * @return The mapped value scaled by the provided level, or 0.0f if no mapping was found.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayEventMapping")
	float GetGameplayEventMapping(FGameplayTag GameplayEventTag, bool& bValueFound, const float InLevelValue = 1.0f) const;
};
