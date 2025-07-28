// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayAbilitySet.h"
#include "BaseCardGameplayAbilitySet.generated.h"

/**
 * @class UBaseCardGameplayAbilitySet
 * @brief Represents a set of gameplay abilities associated with a card in the game.
 *
 * This class is designed to manage and encapsulate a collection of gameplay abilities
 * tied to a specific card. It provides functionality for adding, removing, and managing
 * gameplay abilities and ensures proper integration with the overall gameplay system.
 *
 * The UBaseCardGameplayAbilitySet serves as a container for abilities that define
 * the behaviors, actions, or effects that a card can perform or trigger during
 * gameplay. This allows for modular and extensible ability management.
 *
 * Typical responsibilities of this class include:
 * - Holding and maintaining a registry of gameplay abilities.
 * - Performing initialization of abilities when applied to a card.
 * - Managing lifecycle events such as activation or deactivation of abilities.
 * - Validating ability compatibility with the associated card.
 *
 * This class operates within the Unreal Engine gameplay framework and may leverage
 * Unreal's ability system for robust implementation of gameplay features.
 *
 * Usage of this class allows developers to create dynamic and flexible card behavior
 * systems without tightly coupling to specific gameplay mechanics.
 */
UCLASS()
class GASCOURSE_API UBaseCardGameplayAbilitySet : public UGASCourseGameplayAbilitySet
{
	GENERATED_BODY()

public:
	
	void ActivateCard(UGASCourseAbilitySystemComponent* ASC, FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
};
