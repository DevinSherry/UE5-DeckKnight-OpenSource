// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GASCourseGameplayAbilitySet.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"

void FGASCourseAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FGASCourseAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FGASCourseAbilitySet_GrantedHandles::AddAttributeSet(UGASCourseAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
}

void FGASCourseAbilitySet_GrantedHandles::TakeFromAbilitySystem(UGASCourseAbilitySystemComponent* ASC)
{
	check(ASC);

	if (!ASC->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		ASC->RemoveSpawnedAttribute(Set);
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

UGASCourseGameplayAbilitySet::UGASCourseGameplayAbilitySet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGASCourseGameplayAbilitySet::GiveToAbilitySystem(UGASCourseAbilitySystemComponent* ASC, FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(ASC);

	if (!ASC->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		return;
	}
	
	// Grant the attribute sets.
	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FGASCourseAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];

		if (!IsValid(SetToGrant.AttributeSet))
		{
			continue;
		}

		UGASCourseAttributeSet* NewSet = NewObject<UGASCourseAttributeSet>(ASC->GetOwner(), SetToGrant.AttributeSet);
		ASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}

	// Grant the gameplay abilities.
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FGASCourseAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if (!IsValid(AbilityToGrant.Ability))
		{
			continue;
		}

		FGameplayTag ActiveAbilityTag = AbilityToGrant.InputTag;
		if (AbilityToGrant.InputTag == InputTag_AssignActiveSlot)
		{
			FGameplayTag AvailableSlotTag;
			if (IsActiveAbilitySlotAvailable(ASC, AvailableSlotTag))
			{
				ActiveAbilityTag = AvailableSlotTag;
			}
		}

		UGASCourseGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UGASCourseGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(ActiveAbilityTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// Grant the gameplay effects.
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FGASCourseAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			continue;
		}

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = ASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, ASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}
}

bool UGASCourseGameplayAbilitySet::IsActiveAbilitySlotAvailable(UGASCourseAbilitySystemComponent* ASC, FGameplayTag& AvailableSlotTag) const
{
	if (ASC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASC is null!"));
		return false;
	}
	if (const UGASC_AbilitySystemSettings* AbilitySystemSettings = GetDefault<UGASC_AbilitySystemSettings>())
	{
		FGameplayTagContainer ActiveAbilitySlotTags = AbilitySystemSettings->ActiveAbilitySlotTagContainer;
		if (ActiveAbilitySlotTags.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("ActiveAbilitySlotTagContainer is empty!"));
			return false;
		}

		bool bHasActiveAbilitySlotAvailable = false;
		for (const FGameplayTag& Tag : ActiveAbilitySlotTags)
		{
			if (IsValid(ASC->GetAbilityFromTaggedInput(Tag)))
			{
				continue;
			}
			
			AvailableSlotTag = Tag;
			bHasActiveAbilitySlotAvailable = true;
			break;
		}

		return bHasActiveAbilitySlotAvailable;
			
	}

	return false;
}
