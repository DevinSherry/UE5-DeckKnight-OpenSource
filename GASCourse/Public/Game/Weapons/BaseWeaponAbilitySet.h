// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "BaseWeaponAbilitySet.generated.h"

class UGASCourseAttributeSet;

USTRUCT(BlueprintType)
struct FBaseWeaponAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:

	// Gameplay ability to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGASCourseGameplayAbility> Ability = nullptr;
};

USTRUCT(BlueprintType)
struct FBaseWeaponAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;
};

USTRUCT(BlueprintType)
struct FBaseWeaponAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:
	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGASCourseAttributeSet> AttributeSet;

};

USTRUCT(BlueprintType)
struct FBaseWeaponAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void AddAttributeSet(UGASCourseAttributeSet* Set);

	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

protected:

	// Handles to the granted abilities.
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	// Handles to the granted gameplay effects.
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	// Pointers to the granted attribute sets
	UPROPERTY()
	TArray<TObjectPtr<UGASCourseAttributeSet>> GrantedAttributeSets;
};

/**
 * 
 */
UCLASS()
class GASCOURSE_API UBaseWeaponAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
	UBaseWeaponAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, int32 WeaponLevel,
		FBaseWeaponAbilitySet_GrantedHandles* OutGrantedHandles, 
		UObject* SourceObject = nullptr) const;
	
	UFUNCTION(BlueprintCallable, Category="AbilitySet")
	void GiveToAbilitySystem_BP(UAbilitySystemComponent* ASC, int32 WeaponLevel);
	
	UFUNCTION(BlueprintCallable, Category="AbilitySet")
	void TakeFromAbilitySystem_BP(UAbilitySystemComponent* ASC);
	
public:

	// Gameplay abilities to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FBaseWeaponAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// Gameplay effects to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FBaseWeaponAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// Attribute sets to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
	TArray<FBaseWeaponAbilitySet_AttributeSet> GrantedAttributes;
	
private:
	
	TUniquePtr<FBaseWeaponAbilitySet_GrantedHandles> GrantedHandles;
};
