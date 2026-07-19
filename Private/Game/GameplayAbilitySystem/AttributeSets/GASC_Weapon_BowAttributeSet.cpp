// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASC_Weapon_BowAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GASCourse/GASCourseCharacter.h"

UGASC_Weapon_BowAttributeSet::UGASC_Weapon_BowAttributeSet()
{
}

void UGASC_Weapon_BowAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
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

void UGASC_Weapon_BowAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UGASC_Weapon_BowAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UGASC_Weapon_BowAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UGASC_Weapon_BowAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	const FGameplayEffectSpec& Spec                     = Data.EffectSpec;
	const FGameplayEffectContextHandle& ContextHandle   = Spec.GetContext();
	const FGameplayTagContainer& DynamicTags            = Spec.DynamicGrantedTags;
	const FGameplayTagContainer& AssetTags              = Spec.GetDynamicAssetTags();
	
	// ---------------------------------------------------------------------
	// Resolve target info once
	// ---------------------------------------------------------------------
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AGASCourseCharacter* TargetCharacter = nullptr;
	UGASCourseAbilitySystemComponent* TargetASC = nullptr;

	if (Data.Target.AbilityActorInfo.IsValid())
	{
		const FGameplayAbilityActorInfo* TargetInfo = Data.Target.AbilityActorInfo.Get();
		TargetActor      = TargetInfo->AvatarActor.Get();
		TargetController = TargetInfo->PlayerController.Get();
		TargetCharacter  = Cast<AGASCourseCharacter>(TargetActor);

		if (TargetCharacter)
		{
			TargetASC = Cast<UGASCourseAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent());
		}
	}
	
	// ---------------------------------------------------------------------
	// Resolve source info once
	// ---------------------------------------------------------------------
	AActor* SourceActor = ContextHandle.IsValid() ? ContextHandle.GetInstigator() : nullptr;
	AGASCourseCharacter* SourceCharacter = Cast<AGASCourseCharacter>(SourceActor);
	UGASCourseAbilitySystemComponent* SourceASC =
		ContextHandle.IsValid()
			? Cast<UGASCourseAbilitySystemComponent>(ContextHandle.GetInstigatorAbilitySystemComponent())
			: nullptr;

	// Resolve world once
	UWorld* World = nullptr;
	if (TargetActor)
	{
		World = TargetActor->GetWorld();
	}
	else if (SourceActor)
	{
		World = SourceActor->GetWorld();
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
