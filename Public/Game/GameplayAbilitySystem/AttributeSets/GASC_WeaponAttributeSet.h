// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "GASC_WeaponAttributeSet.generated.h"

/**
 * @class UGASC_WeaponAttributeSet
 * @brief Represents a set of attributes specific to weapons in the gameplay ability system.
 *
 * This class is designed to hold and manage various attributes related to weapons. It integrates
 * with the Unreal Engine Gameplay Ability System to allow dynamic modification and manipulation of these
 * attributes during gameplay.
 *
 * Typically used to define properties such as damage, range, rate of fire, and other weapon-related
 * parameters. It serves as a container for attributes that can be accessed and modified during combat
 * or other gameplay mechanics involving weapons.
 *
 * The attributes in this class are registered with the Gameplay Ability System and are automatically
 * replicated, making them suitable for multiplayer games.
 *
 * Note that any changes to attributes within this class may trigger relevant gameplay events
 * and should be managed carefully to avoid unintended behavior.
 */
UCLASS()
class GASCOURSE_API UGASC_WeaponAttributeSet : public UGASCourseAttributeSet
{
	GENERATED_BODY()
	
public:
	
	UGASC_WeaponAttributeSet();
	
	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData CurrentBowWeaponArrowCount;
	ATTRIBUTE_ACCESSORS(UGASC_WeaponAttributeSet, CurrentBowWeaponArrowCount)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData MaxBowWeaponArrowCount;
	ATTRIBUTE_ACCESSORS(UGASC_WeaponAttributeSet, MaxBowWeaponArrowCount)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData ArrowRegenerationTime = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_WeaponAttributeSet, ArrowRegenerationTime)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData ArrowRegenerationCount = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_WeaponAttributeSet, ArrowRegenerationCount)
	
	
};
