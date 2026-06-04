// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Weapons/GASC_CharacterWeapon_Base.h"
#include "GASC_WeaponInventoryComponent.generated.h"

/**
 * @enum EGASC_WeaponTypes
 * @brief Enumeration representing different types of weapons.
 *
 * This enumeration is used to define various weapon types that can be utilized
 * in the game. It supports the use of Unreal Engine Blueprint and provides display
 * names for each weapon type. The "None" type is hidden in the editor and is used
 * for internal purposes.
 *
 * Enum values:
 * - Sword: Represents a standard sword weapon.
 * - Bow: Represents a bow weapon used for ranged attacks.
 * - Daggers: Represents dual daggers used for close combat.
 * - GreatSword: Represents a larger, heavier sword weapon.
 * - None: Hidden type used to represent no weapon.
 */
UENUM(BlueprintType)
enum class EGASC_WeaponTypes : uint8
{
	Sword		UMETA(DisplayName="Sword and Shield"),
	Bow			UMETA(DisplayName="Bow"),
	Daggers		UMETA(DisplayName="Daggers"),
	GreatSword	UMETA(DisplayName="Great Sword"),
	None		UMETA(Hidden)
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class GASCOURSE_API UGASC_WeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGASC_WeaponInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|WeaponInventory")
	UGASC_CharacterWeapon_Base* GetPrimaryWeaponObject() const;
	
};
