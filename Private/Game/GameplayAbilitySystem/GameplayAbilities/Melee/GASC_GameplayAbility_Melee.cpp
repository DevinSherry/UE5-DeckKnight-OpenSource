// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayAbilities/Melee/GASC_GameplayAbility_Melee.h"

void UGASC_GameplayAbility_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGASCourseCharacter* Character = CastChecked<AGASCourseCharacter>(ActorInfo->AvatarActor.Get());
	MotionWarpingComponent = Character->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarpingComponent)
	{
		UE_LOG(LOG_GASC_GameplayAbility, Error, TEXT("Motion Warp Component is invalid in: %s"), *GetPathNameSafe(this));
		EndAbility(Handle, ActorInfo, CurrentActivationInfo, true, true);
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGASC_GameplayAbility_Melee::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGASC_GameplayAbility_Melee::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
