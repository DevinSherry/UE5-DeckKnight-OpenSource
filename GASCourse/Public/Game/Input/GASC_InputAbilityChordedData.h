// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnhancedInputSubsystemInterface.h"
#include "InputMappingContext.h"
#include "Engine/DataAsset.h"
#include "GASC_InputAbilityChordedData.generated.h"

/**
 * UGASC_InputAbilityChordedData is a data asset used to configure input settings
 * for ability interactions involving input chords in the gameplay ability system.
 * It provides the necessary information for defining input actions, mapping contexts,
 * and priorities for input processing.
 */
UCLASS()
class GASCOURSE_API UGASC_InputAbilityChordedData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Ability Chorded Data")
	UInputAction* AbilityChordInputAction = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Ability Chorded Data")
	UInputMappingContext* AbilityChordInputMappingContext = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Ability Chorded Data")
	int32 AbilityChordInputPriority = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Ability Chorded Data")
	FModifyContextOptions ModifyContextOptions = FModifyContextOptions();
};
