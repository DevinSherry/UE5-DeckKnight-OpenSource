// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActiveCardResourceEventMappingData.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "ActiveCardResourceSettings.generated.h"

/**
 * UActiveCardResourceSettings represents a configurable settings object used to manage
 * the behavior of active card resources in the game. This class inherits from
 * UDeveloperSettings, allowing it to be adjustable through the project settings in
 * Unreal Engine.
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName="Active Card Resource Management Settings"))
class GASCOURSE_API UActiveCardResourceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * ActiveCardResourceEventTags is a container holding gameplay tags associated with
	 * active card resource events. These tags are used to identify and handle specific
	 * events or actions related to the active card resource system within gameplay logic.
	 *
	 * This property is configurable via the project settings and is exposed to both
	 * the editor and Blueprint for easy integration and modification. It is categorized
	 * under "Active Card Resource|Gameplay Tags" for better organization in the settings UI.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Active Card Resource|Gameplay Tags")
	FGameplayTagContainer ActiveCardResourceEventTags;

	/**
	 * ActiveCardResourceEventMappingData is a reference to a data asset of type UActiveCardResourceEventMappingData,
	 * used to define mappings between gameplay event tags and their corresponding scalable values.
	 *
	 * This property is configurable via the project settings and is exposed to both the editor and Blueprint for seamless integration.
	 * It is categorized under "Active Card Resource|Mapping Data" to enhance organization within the settings UI.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Active Card Resource|Mapping Data")
	TSoftObjectPtr<UActiveCardResourceEventMappingData> ActiveCardResourceEventMappingData;
};
