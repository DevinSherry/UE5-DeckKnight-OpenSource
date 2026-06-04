// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Object.h"
#include "GASC_CharacterWeapon_Base.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponLevelChanged);

/**
 * @class UGASC_CharacterWeapon_Base
 * @brief A base class for character weapons in the GAS framework.
 *
 * This class serves as the foundational layer for all character weapon implementations
 * within the Gameplay Ability System (GAS). It provides core functionality, properties,
 * and mechanisms required for handling weapons effectively, ensuring a standard structure
 * for derived weapon classes.
 *
 * @details
 * UGASC_CharacterWeapon_Base defines the shared behavior and attributes for weapons
 * equipped by characters. It facilitates interaction with the Gameplay Ability System
 * and provides hooks and utility functions that can be extended or customized by subclasses.
 *
 * Examples of features include handling weapon-specific interactions, animations,
 * and integration with character abilities. Subclasses can build upon this base to
 * implement specific weapon types such as melee, ranged, or magic-based weapons.
 *
 * Primary use cases:
 * - Equipping/un-equipping weapons for player or AI-controlled characters.
 * - Interfacing with the GAS framework for ability-related functionalities.
 * - Managing weapon-specific logic (e.g., inventory state, attributes, cooldowns).
 */
UCLASS(BlueprintType, Blueprintable, meta=(ShortTooltip="A base object class that stores data about a weapon."))
class GASCOURSE_API UGASC_CharacterWeapon_Base : public UObject
{
	GENERATED_BODY()
	
public:
	UGASC_CharacterWeapon_Base();
	~UGASC_CharacterWeapon_Base();

public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Weapon")
	FVector GetWeaponTrailMidPoint(int32 WeaponIndex = 0) const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Weapon")
	float GetWeaponTrailWidth(int32 WeaponIndex = 0)	const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Weapon")
	FRotator GetWeaponTrailRotation(int32 WeaponIndex = 0) const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Weapon")
	UMeshComponent* GetPrimaryWeaponMeshComponent(int32 WeaponIndex = 0) const;
	
	UFUNCTION(BlueprintCallable, Category = "GASCourse|Weapon")
	bool IsWeaponActive() const
	{
		return bIsWeaponActive;
	}
	
	UFUNCTION(BlueprintCallable, Category = "GASCourse|Weapon")
	void SetWeaponActive(bool IsActive)
	{
		bIsWeaponActive = IsActive;
	}
	
	UFUNCTION(BlueprintPure, Category="GASCourse|Weapon|Leveling")
	int GetWeaponLevel() const
	{
		return WeaponLevel;
	}
	
	UFUNCTION(BlueprintCallable, Category="GASCourse|Weapon|Leveling")
	void IncrementWeaponLevel(int LevelToAdd = 1);

	UFUNCTION(BlueprintCallable, Category="GASCourse|Weapon|Leveling")
	void OverrideWeaponLevel(int LevelOverride);
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Category = "GASCourse|Weapon")
	bool bIsWeaponActive = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "GASCourse|Weapon|Leveling")
	int WeaponLevel = 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "GASCourse|Weapon|Leveling")
	int MaxWeaponLevel = 5;
	
	UPROPERTY(BlueprintReadOnly, Category = "GASCourse|Weapon|Stats")
	UCurveTable* WeaponStatsCurveTable = nullptr;
	
	UPROPERTY(BlueprintAssignable)
	FOnWeaponLevelChanged OnWeaponLevelChanged;
};
