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
	
	if (Attribute == GetMaxWeaponManaAttribute())
	{
		AdjustAttributeForMaxChange(CurrentWeaponMana, MaxWeaponMana, NewValue, GetCurrentWeaponManaAttribute());
	}
	
	if (Attribute == GetCurrentWeaponManaAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxWeaponMana());
	}
	
	if (Attribute == GetWeaponManaRegenDelayAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}
	
	if (Attribute == GetIncomingWeaponManaCritMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}
	
	if (Attribute == GetIncomingWeaponManaKillMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
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
	
	// =====================================================================
	// WEAPON MANA BRANCH
	// =====================================================================
	
	if (Data.EvaluatedData.Attribute == GetIncomingWeaponManaAttribute())
	{
		const float LocalIncomingWeaponMana = GetIncomingWeaponMana();
		SetIncomingWeaponMana(0.0f);
		
		const float OldWeaponMana = CurrentWeaponMana.GetCurrentValue();
		const float NewWeaponMana = OldWeaponMana + LocalIncomingWeaponMana;
		SetCurrentWeaponMana(NewWeaponMana);
	}
	
	if (Data.EvaluatedData.Attribute == GetCurrentWeaponManaAttribute())
	{
		SetCurrentWeaponMana(FMath::Clamp(GetCurrentWeaponMana(), 0.0f, GetMaxWeaponMana()));
	}
	
	if (Data.EvaluatedData.Attribute == GetWeaponManaRegenAttribute())
	{
		SetWeaponManaRegen(FMath::Clamp(GetWeaponManaRegen(), 0.0f, GetMaxWeaponMana()));
	}
}
