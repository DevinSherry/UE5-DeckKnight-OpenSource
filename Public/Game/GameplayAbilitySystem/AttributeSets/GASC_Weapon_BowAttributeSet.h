// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "GASC_Weapon_BowAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_Weapon_BowAttributeSet : public UGASCourseAttributeSet
{
	GENERATED_BODY()
	
public:
	
	UGASC_Weapon_BowAttributeSet();
	
	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData CurrentBowWeaponArrowCount;
	ATTRIBUTE_ACCESSORS(UGASC_Weapon_BowAttributeSet, CurrentBowWeaponArrowCount)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData MaxBowWeaponArrowCount;
	ATTRIBUTE_ACCESSORS(UGASC_Weapon_BowAttributeSet, MaxBowWeaponArrowCount)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData ArrowRegenerationTime = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_Weapon_BowAttributeSet, ArrowRegenerationTime)
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Bow&Arrow|Attributes")
	FGameplayAttributeData ArrowRegenerationCount = 1.0f;
	ATTRIBUTE_ACCESSORS(UGASC_Weapon_BowAttributeSet, ArrowRegenerationCount)
};
