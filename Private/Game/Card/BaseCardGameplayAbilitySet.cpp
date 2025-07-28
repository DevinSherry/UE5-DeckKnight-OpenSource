// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Card/BaseCardGameplayAbilitySet.h"

void UBaseCardGameplayAbilitySet::ActivateCard(UGASCourseAbilitySystemComponent* ASC,
	FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	GiveToAbilitySystem(ASC, OutGrantedHandles, SourceObject);
}
