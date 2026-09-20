// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GASC_ProjectilePoolSettings.generated.h"

class UTargetingPreset;

/**
 * 
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName="GASCourse Projectile Pooling System Settings"))
class GASCOURSE_API UGASC_ProjectilePoolSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UGASC_ProjectilePoolSettings(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Projectile Pool")
	TSoftObjectPtr<UTargetingPreset> RicochetTargetingPreset;

	/**
	 * Projectiles pre-spawned when a gameplay world begins play. Every one is a deferred actor
	 * spawn in a single frame, so raising this trades load-time cost for fewer mid-game spawns
	 * when the pool is exhausted.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Projectile Pool", meta = (ClampMin = "0", UIMin = "0"))
	int32 InitialPoolSize = 100;
};
