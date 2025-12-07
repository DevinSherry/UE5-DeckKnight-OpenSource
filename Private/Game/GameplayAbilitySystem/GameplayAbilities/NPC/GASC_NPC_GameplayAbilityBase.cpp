// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayAbilities/NPC/GASC_NPC_GameplayAbilityBase.h"
#include "Game/Character/NPC/GASCourseNPC_Base.h"


UGASC_NPC_GameplayAbilityBase::UGASC_NPC_GameplayAbilityBase(const FObjectInitializer& ObjectInitializer)
{
	AbilitySlotType = EGASCourseAbilitySlotType::EmptySlot;
}

void UGASC_NPC_GameplayAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	AGASCourseNPC_Base* Character = CastChecked<AGASCourseNPC_Base>(ActorInfo->AvatarActor.Get());
}

void UGASC_NPC_GameplayAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	TRACE_CPUPROFILER_EVENT_SCOPE(NPC_ACTIVATE_ABILITY);
}
