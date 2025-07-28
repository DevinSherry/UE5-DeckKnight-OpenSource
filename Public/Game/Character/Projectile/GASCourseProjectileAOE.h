// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Character/Projectile/GASCourseProjectile.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_AreaOfEffectData.h"
#include "GASCourseProjectileAOE.generated.h"

/**
 * Represents a specialized area-of-effect (AOE) projectile class derived from AGASCourseProjectile.
 * This class adds functionality to handle AOE behaviors, including properties for additional data
 * defining the area-of-effect characteristics of the projectile.
 */
UCLASS()
class GASCOURSE_API AGASCourseProjectileAOE : public AGASCourseProjectile
{
	GENERATED_BODY()

public:
	
	AGASCourseProjectileAOE();

	//-----------------------AOE---------------------//

	/**
	 * Determines whether the projectile has the ability to ricochet upon impact.
	 * Enables behavior where the projectile can bounce off surfaces instead of stopping or being destroyed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|AOE", meta=(ExposeOnSpawn=true))
	UGASC_AreaOfEffectData* AreaOfEffectData = nullptr;
	
};
