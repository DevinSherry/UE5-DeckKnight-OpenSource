// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseCharBaseAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Game/Character/Player/GASCoursePlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GASCourse/GASCourseCharacter.h"

UGASCourseCharBaseAttributeSet::UGASCourseCharBaseAttributeSet()
{

}

void UGASCourseCharBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	//TODO: Clamp movement speed to low/high values to prevent crazy values from getting in.

	if(Attribute == GetAirControlOverrideAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
}

void UGASCourseCharBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	const AGASCoursePlayerState* OwnerPS = Cast<AGASCoursePlayerState>(GetOwningActor());
	if(!OwnerPS)
	{
		return;
	}
	const AGASCourseCharacter* OwnerCharacter = Cast<AGASCourseCharacter>(OwnerPS->GetPawn());
	if(!OwnerCharacter)
	{
		return;
	}

	if(Attribute == GetAirControlOverrideAttribute())
	{
		OwnerCharacter->GetCharacterMovement()->AirControl = NewValue;
	}
}

void UGASCourseCharBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}
