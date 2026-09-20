// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "GASC_WeaponManaInstantGameplayEffect.generated.h"

/**
 * @class UGASC_WeaponManaInstantGameplayEffect
 * @brief A gameplay effect class for instantly modifying mana in the GAS (Gameplay Ability System).
 *
 * This class represents a specific gameplay effect that applies instant changes to the
 * mana attribute of a character or actor. It extends the functionality of the UGASCourseGameplayEffect
 * base class, allowing for precise control over mana adjustments as part of the gameplay ability system.
 *
 * The effect is typically used in scenarios such as weapon abilities or skills that
 * require immediate adjustment of a target's mana pool without applying lingering effects.
 *
 * @note This class should be configured and applied through the GAS framework in Unreal Engine.
 * Ensure the relevant attributes and execution policies are defined for the intended use case.
 */
UCLASS()
class GASCOURSE_API UGASC_WeaponManaInstantGameplayEffect : public UGASCourseGameplayEffect
{
	GENERATED_UCLASS_BODY()
};
