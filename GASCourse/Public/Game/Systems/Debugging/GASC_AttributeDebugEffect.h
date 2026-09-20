// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "GASC_AttributeDebugEffect.generated.h"

/**
 * @class UGASC_AttributeDebugEffect
 * @brief A class that provides debugging functionality for attribute effects.
 *
 * UGASC_AttributeDebugEffect is designed to facilitate the testing and debugging
 * of gameplay attribute effects by allowing developers to simulate and visualize
 * their impact during runtime. It provides utilities that can log, trace, or
 * manipulate attribute-related data for debugging purposes.
 *
 * This class is intended for internal development use and should not be deployed
 * in production builds.
 *
 * Key functionalities of UGASC_AttributeDebugEffect:
 * - Offers tools to monitor attribute-specific changes.
 * - Simulates attribute effects for testing.
 * - Provides detailed debugging logs or feedback.
 *
 * It is often utilized in conjunction with other gameplay systems to ensure
 * attribute effects perform as expected in various scenarios.
 */
UCLASS()
class GASCOURSE_API UGASC_AttributeDebugEffect : public UGASCourseGameplayEffect
{
	GENERATED_UCLASS_BODY()
};
