// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "GASC_DamageGameplayEffect.generated.h"

/**
 * UGASC_DamageGameplayEffect is a specialized Gameplay Effect class used to define
 * and apply damage logic within the context of Unreal Engine's Gameplay Ability System.
 *
 * This class extends UGASCourseGameplayEffect to provide functionality specific to
 * damage-related effects, such as adjusting damage values and incorporating custom
 * logic for how damage effects should be constructed and applied.
 *
 * Key Features:
 * - Supports the configuration of damage effects through the Gameplay Ability System.
 * - Designed to work seamlessly with damage pipelines and effect contexts.
 * - Can be used in conjunction with other Gameplay Effects and Ability System Components.
 *
 * This class is commonly utilized in scenarios where damage calculations must be encapsulated
 * and applied as modular, reusable effects within game systems supporting the Gameplay Ability System.
 */
UCLASS()
class GASCOURSE_API UGASC_DamageGameplayEffect : public UGASCourseGameplayEffect
{
	GENERATED_UCLASS_BODY()
};
