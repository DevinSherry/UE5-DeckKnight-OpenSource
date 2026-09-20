// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "GASC_GrantAbilityOverride.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_GrantAbilityOverride : public UAbilitiesGameplayEffectComponent
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = GrantAbilities)
	FGameplayTag NativeInputActionTag;
	
protected:
	
	virtual void GrantAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const override;
};
