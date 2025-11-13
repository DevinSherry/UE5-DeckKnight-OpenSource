// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"
#include "GASC_NPC_GameplayAbilityBase.generated.h"

/**
 * @class UGASC_NPC_GameplayAbilityBase
 * @brief A base class for handling Non-Player Character (NPC) gameplay abilities within the GAS Course framework.
 *
 * This class extends the UGASCourseGameplayAbility to provide additional functionality specific to NPC abilities.
 * It includes an editable data structure for NPC-specific ability data and overrides important methods to handle
 * the initialization of abilities.
 */
UCLASS()
class GASCOURSE_API UGASC_NPC_GameplayAbilityBase : public UGASCourseGameplayAbility
{
	GENERATED_BODY()
	
public:
	
	UGASC_NPC_GameplayAbilityBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
};
