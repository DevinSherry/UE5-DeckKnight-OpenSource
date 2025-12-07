// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayAbility.h"
#include "GASCourseASCBlueprintLibrary.generated.h"

/** Represents a context for applying damage to an object or character.
 *  Contains information related to the damage event such as the hit result,
 *  damage type, and additional gameplay tags associated with the damage.
 */

USTRUCT(blueprintable)
struct FDamageContext
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite )
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta=(Categories="Damage.Type"))
	FGameplayTag DamageType = DamageType_Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite )
	FGameplayTagContainer GrantedTags;
};

/**
 *  @struct FDamageOverTimeContext
 *  @brief Structure representing the context for damage over time.
 *
 *  Structure that holds the parameters necessary for applying damage over time.
 *
 *  @remark This structure is blueprintable.
 */
USTRUCT(Blueprintable)
struct FEffectOverTimeContext
{
	GENERATED_USTRUCT_BODY()

public:
	/** Defines the interval, in seconds, at which a periodic gameplay effect is applied.
	 *  A negative value indicates that the effect is not periodic and will not execute
	 *  at intervals. If set to a valid positive value, the effect will trigger periodically
	 *  with the specified time gap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectPeriod = -1.0f;

	/** Specifies the total duration, in seconds, for which a gameplay effect remains active.
	 *  A negative value indicates that the effect has an indefinite duration or does not execute over a specific time period.
	 *  If set to a positive value, the effect will be active for the specified time span.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectDuration = -1.0f;

	/** Indicates whether a gameplay effect should be applied immediately upon its activation.
	 *  If true, the effect will execute its logic as soon as it is activated, in addition to any periodic or duration-based execution.
	 *  If false, the effect will only execute based on its periodic or duration settings, if applicable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite )
	bool bApplyEffectOnApplication = true;

	/** Determines whether a value should be distributed evenly over the total duration of the effect.
	 *  If true, the value will be applied proportionally throughout the entire duration, rather than at discrete intervals.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite )
	bool bApplyValueOverTotalDuration = false;
};

/**
 * UGASCourseASCBlueprintLibrary is a blueprint library that provides utility functions for applying damage to target actors.
 */

UCLASS()
class GASCOURSE_API UGASCourseASCBlueprintLibrary : public UAbilitySystemBlueprintLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "GASCourse|AbilitySystem|Damage")
	static bool FindDamageTypeTagInContainer(const FGameplayTagContainer& InContainer, FGameplayTag& DamageTypeTag);

	/**
	 * Retrieves the gameplay ability slot type from the specified ability spec handle.
	 *
	 * @param AbilitySystem The ability system component to retrieve the ability spec from.
	 * @param AbilitySpecHandle The handle of the ability spec to retrieve the slot type from.
	 * @return The gameplay ability slot type associated with the specified ability spec handle.
	 */
	UFUNCTION(BlueprintPure, Category = "GASCourse|AbilitySystem|GameplayAbility")
	static EGASCourseAbilitySlotType GetGameplayAbilitySlotTypeFromHandle(const UAbilitySystemComponent* AbilitySystem, const FGameplayAbilitySpecHandle& AbilitySpecHandle);

	UFUNCTION(BlueprintCallable, Category = "GASCourse|AbilitySystem|GameplayAbility")
	static void GetAllAbilitiesofAbilitySlotType(const UAbilitySystemComponent* AbilitySystem, EGASCourseAbilitySlotType AbilitySlot, TArray<FGameplayAbilitySpecHandle>& OutAbilityHandles);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GASCourse|AbilitySystem|Damage")
	static void SendGameplayEventToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, FGameplayTag EventTag, FGameplayEventData Payload);

	/**
	 * Retrieves the gameplay attribute from a given modifier struct.
	 *
	 * @param ModifierInfo The gameplay modifier info struct.
	 * @return The gameplay attribute.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASCourse|AbilitySystem|GameplayEffect")
	static FGameplayAttribute GetGameplayAttributeFromModifierStruct(const FGameplayModifierInfo& ModifierInfo);

	/**
	 * Calculates the magnitude of a modifier for a given gameplay effect.
	 *
	 * @param InGameplayEffect The handle to the gameplay effect.
	 * @param ModifierIdx The index of the modifier to calculate the magnitude for.
	 * @param bFactorInStackCount Specifies whether to factor in the stack count of the gameplay effect.
	 *
	 * @return The magnitude of the specified modifier. If the modifier is not found or the gameplay effect is invalid,
	 *         returns 0.0f.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASCourse|AbilitySystem|GameplayEffect")
	static float GetModifierMagnitudeAtIndex(FActiveGameplayEffectHandle InGameplayEffect, int32 ModifierIdx, bool bFactorInStackCount);

	/**
	 * Retrieves the gameplay effect specification handle associated with the given active gameplay effect handle.
	 *
	 * @param InGameplayEffect The active gameplay effect handle for which to retrieve the gameplay effect specification handle.
	 * @return The gameplay effect specification handle associated with the given active gameplay effect handle.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASCourse|AbilitySystem|GameplayEffect")
	static FGameplayEffectSpec GetSpecHandleFromGameplayEffect(FActiveGameplayEffectHandle InGameplayEffect);

	/**
	 * Retrieves the period of a gameplay effect.
	 *
	 * @param InGameplayEffect The handle to the active gameplay effect.
	 * @return The period of the gameplay effect.
	 */
	UFUNCTION(BlueprintPure, Category =  "GASCourse|AbilitySystem|GameplayEffect")
	static float GetPeriodFromGameplayEffect(FActiveGameplayEffectHandle InGameplayEffect);

	UFUNCTION(BlueprintCallable, Category = "GASCourse|AbilitySystem|GameplayAbility")
	static bool GrantAbilityToInputTag(UAbilitySystemComponent* InASC, TSubclassOf<UGASCourseGameplayAbility> Ability, int AbilityLevel, FGameplayTag InputTag);
};
