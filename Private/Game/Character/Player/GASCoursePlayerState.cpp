// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Player/GASCoursePlayerState.h"
#include "Engine/ActorChannel.h"
#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"

AGASCoursePlayerState::AGASCoursePlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGASCourseAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	DeckManagerComponent = CreateDefaultSubobject<UDeckManagerComponent>(TEXT("DeckManagerComponent"));

	SetNetUpdateFrequency(100.0f);
}

UGASCourseAbilitySystemComponent* AGASCoursePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


void AGASCoursePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
}
