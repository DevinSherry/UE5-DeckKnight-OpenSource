// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlueprintLibraries/GameplayAbilitySystem/GASCourseASCBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Game/Systems/Damage/GASCourseDamageExecution.h"

bool UGASCourseASCBlueprintLibrary::FindDamageTypeTagInContainer(const FGameplayTagContainer& InContainer, FGameplayTag& DamageTypeTag)
{
	if(InContainer.HasTag(FGameplayTag::RequestGameplayTag(FName("Damage.Type"))))
	{
		for(FGameplayTag Tag : InContainer.GetGameplayTagArray())
		{
			if(Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Damage.Type"))))
			{
				DamageTypeTag = Tag;
				return true;
			}
		}
	}
	
	return false;
}

EGASCourseAbilitySlotType UGASCourseASCBlueprintLibrary::GetGameplayAbilitySlotTypeFromHandle(
	const UAbilitySystemComponent* AbilitySystem, const FGameplayAbilitySpecHandle& AbilitySpecHandle)
{
	EGASCourseAbilitySlotType AbilitySlot = EGASCourseAbilitySlotType::EmptySlot;
	// validate the ASC
	if (!AbilitySystem)
	{
		return AbilitySlot;
	}

	// get and validate the ability spec
	const FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec)
	{
		return AbilitySlot;
	}

	// try to get the ability instance
	if(const UGASCourseGameplayAbility* AbilityInstance = Cast<UGASCourseGameplayAbility>(AbilitySpec->GetPrimaryInstance()))
	{
		AbilitySlot = AbilityInstance->GetAbilitySlotType();
	}

	return AbilitySlot;
}

void UGASCourseASCBlueprintLibrary::GetAllAbilitiesofAbilitySlotType(const UAbilitySystemComponent* AbilitySystem,  EGASCourseAbilitySlotType AbilitySlot, 
	TArray<FGameplayAbilitySpecHandle>& OutAbilityHandles)
{
	if(AbilitySystem)
	{
		OutAbilityHandles.Empty(AbilitySystem->GetActivatableAbilities().Num());
		for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
		{
			if(GetGameplayAbilitySlotTypeFromHandle(AbilitySystem, Spec.Handle) == AbilitySlot)
			{
				// add the spec handle to the list
				OutAbilityHandles.Add(Spec.Handle);
			}
		}
	}
}

void UGASCourseASCBlueprintLibrary::SendGameplayEventToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle,
	FGameplayTag EventTag, FGameplayEventData Payload)
{
	TArray<AActor*> Targets = GetAllActorsFromTargetData(TargetHandle);
	for(AActor* Target : Targets)
	{
		SendGameplayEventToActor(Target, EventTag, Payload);
	}
}

FGameplayAttribute UGASCourseASCBlueprintLibrary::GetGameplayAttributeFromModifierStruct(
	const FGameplayModifierInfo& ModifierInfo)
{
	FGameplayAttribute Attribute;

	if(ModifierInfo.Attribute.IsValid())
	{
		Attribute = ModifierInfo.Attribute;
	}

	return Attribute;
}

float UGASCourseASCBlueprintLibrary::GetModifierMagnitudeAtIndex(FActiveGameplayEffectHandle InGameplayEffect, int32 ModifierIdx,
	bool bFactorInStackCount)
{
	float OutModifierMagnitude = 0.0f;
	
	const FGameplayEffectSpec& Spec = GetSpecHandleFromGameplayEffect(InGameplayEffect);
	if(Spec.Def)
	{
		Spec.Def->Modifiers[ModifierIdx].ModifierMagnitude.AttemptCalculateMagnitude(Spec, OutModifierMagnitude);
	}
	
	return OutModifierMagnitude;
}

FGameplayEffectSpec UGASCourseASCBlueprintLibrary::GetSpecHandleFromGameplayEffect(FActiveGameplayEffectHandle InGameplayEffect)
{
	FGameplayEffectSpec OutSpec;
	if(const UAbilitySystemComponent* AbilitySystemComponent = InGameplayEffect.GetOwningAbilitySystemComponent())
	{
		if(const FActiveGameplayEffect* ActiveGameplayEffect = AbilitySystemComponent->GetActiveGameplayEffect(InGameplayEffect))
		{
			OutSpec = ActiveGameplayEffect->Spec;
		}
	}

	return OutSpec;
}

float UGASCourseASCBlueprintLibrary::GetPeriodFromGameplayEffect(FActiveGameplayEffectHandle InGameplayEffect)
{
	float OutPeriod = 0.0f;

	const FGameplayEffectSpec& Spec = GetSpecHandleFromGameplayEffect(InGameplayEffect);
	if(Spec.Def)
	{
		OutPeriod = Spec.GetPeriod();
	}
	
	return OutPeriod;
}

bool UGASCourseASCBlueprintLibrary::GrantAbilityToInputTag(UAbilitySystemComponent* InASC, TSubclassOf<UGASCourseGameplayAbility> Ability, int AbilityLevel,
                                                           FGameplayTag InputTag)
{
	if (!Ability)
	{
		return false;
	}

	FGameplayAbilitySpec AbilitySpec(Ability, AbilityLevel);
	AbilitySpec.SourceObject = InASC;
	AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);

	const FGameplayAbilitySpecHandle AbilitySpecHandle = InASC->GiveAbility(AbilitySpec);

	return true;
}

