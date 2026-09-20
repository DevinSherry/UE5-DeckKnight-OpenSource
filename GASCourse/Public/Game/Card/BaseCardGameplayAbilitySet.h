// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/GASCourseGameplayAbilitySet.h"
#include "Engine/StreamableManager.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Ability/GASC_AbilityDurationEffect.h"
#include "BaseCardGameplayAbilitySet.generated.h"

class UAssetManager;

USTRUCT()
struct FDurationProperties
{
	GENERATED_BODY()

	/**
	 * @variable DurationEffect
	 * @brief Represents the core duration value for a time-based gameplay effect.
	 *
	 * This variable determines the length of time a specific effect or ability remains active
	 * once triggered in the game. It serves as a fundamental parameter for managing time-limited
	 * effects, such as buffs, debuffs, or status alterations, within the game's mechanics.
	 *
	 * The value of this variable governs the temporal behavior of associated effects,
	 * enabling better control over the scope and timing of gameplay events.
	 *
	 * Key characteristics of this variable include:
	 * - Defining the active timeframe for a triggered effect.
	 * - Supporting the timing and orchestration of gameplay elements.
	 * - Ensuring consistency and balance by regulating effect durations.
	 *
	 * This variable typically works in conjunction with activation conditions, scaling factors,
	 * or periodic updates to create complex and dynamic gameplay interactions.
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGASC_AbilityDurationEffect> DurationEffect = nullptr;

	/**
	 * @variable DurationEffectAppliedAtLevelMin
	 * @brief Defines the minimum level required for a duration-based effect to be applied.
	 *
	 * This variable is used to represent the lowest level at which a duration effect can
	 * be activated or applied within the game's mechanics. It serves as a threshold to restrict
	 * certain gameplay effects based on the entity's or player's level.
	 *
	 * The value of this variable directly influences the activation logic for duration effects,
	 * ensuring that they are conditioned on progression or scaling factors tied to the game's level system.
	 *
	 * Key characteristics of this variable include:
	 * - Establishing a baseline for level-dependent duration effects.
	 * - Supporting conditional gameplay logic tied to progression systems.
	 * - Contributing to the balance and scalability of game mechanics.
	 *
	 * This variable typically interacts with other level-based systems to determine if
	 * additional game features or effects should be triggered.
	 */
	UPROPERTY(EditDefaultsOnly)
	float DurationEffectAppliedAtLevelMin = 0.0f;
};

USTRUCT()
struct FLoadedAbility
{
	GENERATED_BODY()
	int32 SourceIndex = INDEX_NONE;
	TSubclassOf<UGASCourseGameplayAbility> GameplayAbilityClass;
};

USTRUCT()
struct FLoadedGameplayEffect
{
	GENERATED_BODY()
	int32 SourceIndex = INDEX_NONE;
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
};
	

USTRUCT(BlueprintType)
struct FGASCourseCardAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UGASCourseGameplayAbility> GameplayAbility;

	UPROPERTY(EditDefaultsOnly)
	EGASCourseAbilitySlotType AbilitySlotType = EGASCourseAbilitySlotType::EmptySlot;
};

USTRUCT(BlueprintType)
struct FGASCourseCardAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UGameplayEffect> GameplayEffect;
};

USTRUCT(BlueprintType)
struct FGASCourseCardAbilitySet_AttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGASCourseAttributeSet> AttributeSet;
};

USTRUCT(BlueprintType)
struct FGASCourseCardAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void AddAttributeSet(UGASCourseAttributeSet* Set);
	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY() TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	UPROPERTY() TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
	UPROPERTY() TArray<TObjectPtr<UGASCourseAttributeSet>> GrantedAttributeSets;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardDataLoaded, bool, bLoadSuccess);

/**
 * @class UBaseCardGameplayAbilitySet
 * @brief Represents a set of gameplay abilities associated with a card in the game.
 *
 * This class is designed to manage and encapsulate a collection of gameplay abilities
 * tied to a specific card. It provides functionality for adding, removing, and managing
 * gameplay abilities and ensures proper integration with the overall gameplay system.
 *
 * The UBaseCardGameplayAbilitySet serves as a container for abilities that define
 * the behaviors, actions, or effects that a card can perform or trigger during
 * gameplay. This allows for modular and extensible ability management.
 *
 * Typical responsibilities of this class include:
 * - Holding and maintaining a registry of gameplay abilities.
 * - Performing initialization of abilities when applied to a card.
 * - Managing lifecycle events such as activation or deactivation of abilities.
 * - Validating ability compatibility with the associated card.
 *
 * This class operates within the Unreal Engine gameplay framework and may leverage
 * Unreal's ability system for robust implementation of gameplay features.
 *
 * Usage of this class allows developers to create dynamic and flexible card behavior
 * systems without tightly coupling to specific gameplay mechanics.
 */
UCLASS()
class GASCOURSE_API UBaseCardGameplayAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UBaseCardGameplayAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, int32 CardLevel,
		FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles, 
		UObject* SourceObject = nullptr) const;
	
	UFUNCTION(BlueprintCallable, Category="Card")
	bool ActivateCard(UAbilitySystemComponent* ASC, int32 CardLevel = 1, bool bAutoLoadThenActivate = true);
	
	UFUNCTION(BlueprintCallable, Category="Card")
	void LoadCardDataAsync();
	
	UFUNCTION(BlueprintCallable, Category="Card")
	bool IsCardDataLoaded() const { return bCardDataLoaded; }
	
	UFUNCTION(BlueprintCallable, Category="Card")
	void OverrideCardLevel(float InCardLevelOverride);
	
	UPROPERTY(BlueprintAssignable, Category="Card")
	FOnCardDataLoaded OnCardDataLoaded;
	
	// Gameplay abilities to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=GameplayAbility))
	TArray<FGASCourseCardAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// Gameplay effects to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FGASCourseCardAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// Attribute sets to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
	TArray<FGASCourseCardAbilitySet_AttributeSet> GrantedAttributes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Card|Duration")
	FDurationProperties CardDurationProperties;

	
private:
	
	TArray<FLoadedAbility> LoadedAbilities;
	TArray<FLoadedGameplayEffect> LoadedGameplayEffects;
	
	bool bCardDataLoaded = false;
	bool bCardDataLoading = false;
	int32 CardLevel_Internal = 1;
	
	TSharedPtr<FStreamableHandle> CardDataLoadHandle;
	
	// If ActivateCard called before load, we can optionally auto-grant after load
	struct FPendingActivation
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FGASCourseAbilitySet_GrantedHandles* Handles = nullptr; // caller-owned
		TWeakObjectPtr<UObject> SourceObject;
	};

	// NOTE: This is shared per DataAsset, so keep it minimal.
	// If you expect multiple ASCs to request activation before load completes,
	// you can queue them here.
	TArray<FPendingActivation> PendingActivations;

private:
	void StartAsyncLoad_Internal();
	void OnAsyncLoadComplete_Internal(bool bSuccess);
	void BuildLoadedCaches_FromSoftRefs();
	void FlushPendingActivations();
	
};
