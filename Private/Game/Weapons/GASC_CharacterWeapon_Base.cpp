// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Weapons/GASC_CharacterWeapon_Base.h"


UGASC_CharacterWeapon_Base::UGASC_CharacterWeapon_Base()
{
}

UGASC_CharacterWeapon_Base::~UGASC_CharacterWeapon_Base()
{
}

void UGASC_CharacterWeapon_Base::IncrementWeaponLevel(int LevelToAdd)
{
	WeaponLevel = FMath::Clamp(WeaponLevel + LevelToAdd, 1, MaxWeaponLevel);
	OnWeaponLevelChanged.Broadcast();
}

void UGASC_CharacterWeapon_Base::OverrideWeaponLevel(int LevelOverride)
{
	WeaponLevel = FMath::Clamp(LevelOverride, 1, MaxWeaponLevel);
	OnWeaponLevelChanged.Broadcast();
}
