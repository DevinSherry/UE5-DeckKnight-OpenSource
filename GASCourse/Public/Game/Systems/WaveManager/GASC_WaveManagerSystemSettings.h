// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GASC_WaveManagerSystemSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName="Wave Manager System Settings"))
class GASCOURSE_API UGASC_WaveManagerSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Wave Manager|Enemy")
	TArray<TSoftClassPtr<APawn>> EnemyList;
	
};
