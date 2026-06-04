// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Weapons/BaseWeaponAbilitySet.h"


void FBaseWeaponAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FBaseWeaponAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FBaseWeaponAbilitySet_GrantedHandles::AddAttributeSet(UGASCourseAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
}

void FBaseWeaponAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
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

UBaseWeaponAbilitySet::UBaseWeaponAbilitySet(const FObjectInitializer& ObjectInitializer)
{
}

void UBaseWeaponAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, int32 WeaponLevel,
	FBaseWeaponAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
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
		const FBaseWeaponAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];

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
		const FBaseWeaponAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if (!IsValid(AbilityToGrant.Ability))
		{
			continue;
		}

		UGASCourseGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UGASCourseGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, WeaponLevel);
		AbilitySpec.SourceObject = SourceObject;

		const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// Grant the gameplay effects.
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FBaseWeaponAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			continue;
		}

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = ASC->ApplyGameplayEffectToSelf(GameplayEffect, WeaponLevel, ASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}
}

void UBaseWeaponAbilitySet::GiveToAbilitySystem_BP(UAbilitySystemComponent* ASC, int32 WeaponLevel)
{
	if (!GrantedHandles)
	{
		GrantedHandles = MakeUnique<FBaseWeaponAbilitySet_GrantedHandles>();
	}
	GiveToAbilitySystem(ASC, WeaponLevel, GrantedHandles.Get(), ASC->GetOwnerActor());
}

void UBaseWeaponAbilitySet::TakeFromAbilitySystem_BP(UAbilitySystemComponent* ASC)
{
	if (GrantedHandles)
	{
		GrantedHandles->TakeFromAbilitySystem(ASC);
		GrantedHandles.Reset();
	}
}
