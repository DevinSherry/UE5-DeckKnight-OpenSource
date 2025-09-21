// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "GASC_CardResourcesAttributeSet.generated.h"

/**
 * @class UGASC_CardResourcesAttributeSet
 * @brief Defines the attribute set for card resources in the system.
 *
 * The UGASC_CardResourcesAttributeSet class is responsible for encapsulating
 * and managing data related to card resource attributes. This class provides
 * a framework for handling and manipulating resource values such as health,
 * mana, and other custom attributes tied to specific cards within the system.
 * It plays a foundational role in implementing attribute-driven mechanics.
 *
 * The attribute set integrates into the larger gameplay architecture and works
 * in conjunction with other systems to ensure proper replication, modification,
 * and calculation of resource-based parameters. It can respond to gameplay
 * events or changes, ensuring dynamic updates throughout the lifecycle of the
 * card or object utilizing this attribute set.
 *
 * The core responsibilities of this class involve:
 * - Defining the types of resources specific to cards.
 * - Managing base and current resource values.
 * - Enabling resource interactions, such as modification or regeneration.
 * - Supporting replication for multiplayer setups and synchronization.
 *
 * This attribute set is to be extended or used directly depending on the
 * customization needs of the resources in the specific gameplay scope.
 */
UCLASS()
class GASCOURSE_API UGASC_CardResourcesAttributeSet : public UGASCourseAttributeSet
{
	GENERATED_BODY()

public:

	UGASC_CardResourcesAttributeSet();

	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData CurrentCardEnergy;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, CurrentCardEnergy)

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData MaximumCardEnergy;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, MaximumCardEnergy)

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData CurrentCardEnergyXP;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, CurrentCardEnergyXP)

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData MaximumCardEnergyXP;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, MaximumCardEnergyXP)

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData PassiveCardEnergyXPGain;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, PassiveCardEnergyXPGain)

	UPROPERTY(BlueprintReadOnly, Category = "Character Card Resource Attribute Set")
	FGameplayAttributeData IncomingCardEnergyXP;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, IncomingCardEnergyXP)

	UPROPERTY(BlueprintReadOnly, Category = "Character Health Attributes")
	FGameplayAttributeData ActiveCardEnergyMultiplier = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, ActiveCardEnergyMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "Character Health Attributes")
	FGameplayAttributeData OnKillActiveCardEnergyMultiplier = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_CardResourcesAttributeSet, OnKillActiveCardEnergyMultiplier)
	
};
