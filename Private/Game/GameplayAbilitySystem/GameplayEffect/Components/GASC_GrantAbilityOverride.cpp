// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Components/GASC_GrantAbilityOverride.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"


void UGASC_GrantAbilityOverride::GrantAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const
{
	UAbilitySystemComponent* ASC = ActiveGEHandle.GetOwningAbilitySystemComponent();
	if (!ensure(ASC))
	{
		UE_LOG(LogGameplayEffects, Error, TEXT("%s was passed an ActiveGEHandle %s which did not have a valid associated AbilitySystemComponent"), ANSI_TO_TCHAR(__func__), *ActiveGEHandle.ToString());
		return;
	}

	if (ASC->bSuppressGrantAbility)
	{
		UE_LOG(LogGameplayEffects, Warning, TEXT("%s suppressed by %s bSuppressGrantAbility"), *GetName(), *ASC->GetName());
		return;
	}

	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveGEHandle);
	if (!ActiveGE)
	{
		UE_LOG(LogGameplayEffects, Warning, TEXT("ActiveGEHandle %s did not corresponds to Active Gameplay Effect on %s. This could potentially happen if you remove the GE during the application of other GE's"), *ActiveGEHandle.ToString(), *ASC->GetName());
		return;
	}
	const FGameplayEffectSpec& ActiveGESpec = ActiveGE->Spec;

	const TArray<FGameplayAbilitySpec>& AllAbilities = ASC->GetActivatableAbilities();
	for (const FGameplayAbilitySpecConfig& AbilityConfig : GrantAbilityConfigs)
	{
		// Check that we're configured
		const UGameplayAbility* AbilityCDO = AbilityConfig.Ability.GetDefaultObject();
		if (!AbilityCDO)
		{
			continue;
		}

		// Only do this if we haven't assigned the ability yet! This prevents cases where stacking GEs
		// would regrant the ability every time the stack was applied
		const bool bAlreadyGrantedAbility = AllAbilities.ContainsByPredicate([ASC, AbilityCDO, &ActiveGEHandle](FGameplayAbilitySpec& Spec) { return Spec.Ability == AbilityCDO && Spec.GameplayEffectHandle == ActiveGEHandle; });
		if (bAlreadyGrantedAbility)
		{
			continue;
		}

		const FString ContextString = FString::Printf(TEXT("%s for %s from %s"), ANSI_TO_TCHAR(__func__), *AbilityCDO->GetName(), *GetNameSafe(ActiveGESpec.Def));
		const int32 Level = static_cast<int32>(AbilityConfig.LevelScalableFloat.GetValueAtLevel(ActiveGESpec.GetLevel(), &ContextString));

		// Now grant that ability to the owning actor
		FGameplayAbilitySpec AbilitySpec{ AbilityConfig.Ability, Level, AbilityConfig.InputID, ActiveGESpec.GetEffectContext().GetSourceObject() };
		AbilitySpec.SetByCallerTagMagnitudes = ActiveGESpec.SetByCallerTagMagnitudes;
		AbilitySpec.GameplayEffectHandle = ActiveGEHandle;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(NativeInputActionTag);
		ASC->GiveAbility(AbilitySpec);
	}
}
