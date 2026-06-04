// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASC_WeaponAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GASCourse/GASCourseCharacter.h"


UGASC_WeaponAttributeSet::UGASC_WeaponAttributeSet()
{
}

void UGASC_WeaponAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if(Attribute == GetMaxBowWeaponArrowCountAttribute())
	{
		AdjustAttributeForMaxChange(CurrentBowWeaponArrowCount, MaxBowWeaponArrowCount, NewValue, GetCurrentBowWeaponArrowCountAttribute());
	}
	
	if (Attribute == GetCurrentBowWeaponArrowCountAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxBowWeaponArrowCount());
	}
	
}

void UGASC_WeaponAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UGASC_WeaponAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UGASC_WeaponAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UGASC_WeaponAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AGASCourseCharacter* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AGASCourseCharacter>(TargetActor);
	}
	
	if (Data.EvaluatedData.Attribute == GetCurrentBowWeaponArrowCountAttribute())
	{
		SetCurrentBowWeaponArrowCount(FMath::Clamp(GetCurrentBowWeaponArrowCount(), 0.0f, GetMaxBowWeaponArrowCount()));
	}
	if (Data.EvaluatedData.Attribute == GetArrowRegenerationCountAttribute())
	{
		SetArrowRegenerationCount(FMath::Clamp(GetArrowRegenerationCount(), 0.0f, GetMaxBowWeaponArrowCount()));
	}
	
	if (Data.EvaluatedData.Attribute == GetArrowRegenerationTimeAttribute())
	{
		SetArrowRegenerationTime(FMath::Clamp(GetArrowRegenerationTime(), 0.1f, 5.0f));
	}
}
