// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
//#include "Game/Character/Player/GASCoursePlayerCharacter.h"
//#include "Game/Character/Player/GASCoursePlayerController.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "GASCourseGameplayAbility.generated.h"

class AGASCoursePlayerCharacter;
class AGASCoursePlayerController;

/*
 * Delegate fired when ability is committed, returns whether commit was successful
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGASCourseAbilityCommitSignature, bool, CommitAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGASCourseAbilityClearedSignature, UGameplayAbility*, AbilityCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGASCourseAbilityCooldownCommitSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGASCourseAbilityDurationRemoved);

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_GameplayAbility, Log, All);

/**
 * EGASCourseAbilityActivationPolicy
 *
 *	Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EGASCourseAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

UENUM(BlueprintType)
enum class EGASCourseAbilityType : uint8
{
	
	AimCast,
	Duration,
	Instant,
	Stack
};

/**
 * Represents the different slot types for a gameplay ability.
 *
 * The slot types are used to categorize abilities based on their usage.
 */
UENUM(BlueprintType)
enum class EGASCourseAbilitySlotType : uint8
{
	
	ActiveAbilitySlot,
	EvasiveAbilitySlot,
	PassiveAbilitySlot,
	InstantAbilitySlot,
	EmptySlot
};


USTRUCT(BlueprintType)
struct FGrantedCardAbilityConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Ability Configuration")
	EGASCourseAbilityType AbilityType = EGASCourseAbilityType::Instant;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Ability Configuration")
	EGASCourseAbilitySlotType SlotType = EGASCourseAbilitySlotType::EmptySlot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Ability Configuration")
	TSubclassOf<UGameplayEffect> DurationEffect;
};

/**
 * UGASCourseGameplayAbility
 *
 * The base gameplay ability class used by this project. It extends UGameplayAbility
 * and includes additional features and properties specific to the GASCourse project,
 * such as support for stackable abilities, custom activation policies, and integration
 * with the project's player and ability system components.
 */
UCLASS(Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class GASCOURSE_API UGASCourseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
	friend class UGASCourseAbilitySystemComponent;

public:

	UPROPERTY(BlueprintAssignable)
	FGASCourseAbilityCommitSignature OnAbilityCommitDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FGASCourseAbilityClearedSignature OnAbilityClearedDelegate;

	UFUNCTION(BlueprintPure, Category = "GASCourse|Ability|Tags")
	void GetAbilityCooldownTags(FGameplayTagContainer& CooldownTags) const;

	/**
	 * Retrieves the duration tags of the ability.
	 *
	 * This method populates the provided `DurationTags` container with tags that represent
	 * the duration-related properties or effects of the gameplay ability.
	 *
	 * @param DurationTags A reference to a `FGameplayTagContainer` that will be filled with
	 * duration-related tags applicable to the ability.
	 */
	UFUNCTION(BlueprintPure, Category = "GASCourse|Ability|Tags")
	virtual void GetAbilityDurationTags(FGameplayTagContainer& DurationTags) const;

	/**
	 * @brief Get the granted by effect duration.
	 *
	 * This method returns the duration of the effect granted by the ability.
	 *
	 * @return The duration of the effect granted by the ability.
	 */
	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetGrantedbyEffectDuration() const;
	
public:

	UGASCourseGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability")
	UGASCourseAbilitySystemComponent* GetGASCourseAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability")
	AGASCoursePlayerController* GetGASCoursePlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability")
	AController* GetControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability")
	AGASCoursePlayerCharacter* GetGASCouresPlayerCharacterFromActorInfo() const;

	UFUNCTION(BlueprintPure)
	FGameplayTagContainer GetDynamicAbilityTags()const {return GetCurrentAbilitySpec()->GetDynamicSpecSourceTags();}

	EGASCourseAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	/**
	 * @brief Get the type of ability slot.
	 *
	 * This method returns the type of ability slot that the ability belongs to.
	 *
	 * @return The ability slot type (EGASCourseAbilitySlotType) of the ability.
	 */
	EGASCourseAbilitySlotType GetAbilitySlot() const {return AbilitySlotType; }
	void SetAbilitySlot(EGASCourseAbilitySlotType InAbilitySlotType) { AbilitySlotType = InAbilitySlotType; }
	
	EGASCourseAbilityType GetAbilityType() const {return AbilityType; }
	void SetAbilityType(EGASCourseAbilityType InAbilityType) { AbilityType = InAbilityType; }

	/**
	 * SetAbilityIcon
	 *
	 * Updates the icon representing a specific ability.
	 *
	 * @param InAbilityIcon The name of the ability whose icon is being set.
	 * @param iconPath The file path to the new icon image.
	 */
	void SetAbilityIcon(TSoftObjectPtr<UTexture2D> InAbilityIcon) { AbilityIcon = InAbilityIcon; }
	TSoftObjectPtr<UTexture2D> GetAbilityIcon() const { return AbilityIcon; }

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	UFUNCTION(BlueprintPure, Category = "GASCourse|Ability|Cost")
	FScalableFloat GetAbilityCost() const { return AbilityCost; }

	/**
	 * GetInputDirection
	 *
	 * Retrieves the input direction.
	 *
	 * @param bReturnActorForward If true, returns the actor's forward direction when no input is detected. Otherwise, return zero vector.
	 * @return The vector representing the input direction.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASCourse|Ability|Input", Meta = (DisplayName = "Get Input Direction"))
	virtual FVector GetInputDirection() const;
	
public:
	
	//Callback for when applied duration effect is removed. Ability must be of type EGASCourseAbilityType::Duration
	UFUNCTION()
	void DurationEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo);

	/**
	 * GetDurationGameplayEffect
	 *
	 * Retrieves the gameplay effect class associated with the duration effect.
	 *
	 * @return The TSubclassOf object representing the duration gameplay effect.
	 */
	UFUNCTION()
	FORCEINLINE TSubclassOf<UGameplayEffect> GetDurationGameplayEffect() const {return DurationEffect;}

	/**
	 * SetDurationGameplayEffect
	 *
	 * Sets the duration gameplay effect for this ability.
	 *
	 * @param InDurationEffect The gameplay effect class to set as the duration effect.
	 */
	UFUNCTION()
	FORCEINLINE void SetDurationGameplayEffect(TSubclassOf<UGameplayEffect> InDurationEffect) {DurationEffect = InDurationEffect;}
	
protected:
	
	//The duration effect to be applied, dictates how long the ability will last for.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Duration", meta=(EditCondition="AbilityType==EGASCourseAbilityType::Duration", EditConditionHides))
	TSubclassOf<UGameplayEffect> DurationEffect;

	/*Auto apply the duration effect on ability activation, otherwise use function Apply Duration Effect**/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Duration", meta=(EditCondition="AbilityType==EGASCourseAbilityType::Duration", EditConditionHides))
	bool bAutoApplyDurationEffect;

	/*Manually apply either class duration effect, or custom duration effect**/
	UFUNCTION(BlueprintCallable, Category = "GASCourse|Ability|Duration")
	UPARAM(DisplayName= "bDurationEffectApplied")
	bool ApplyDurationEffect();

	UFUNCTION()
	void OnAbilityInputPressed(float InTimeWaited);

	/**
 * @brief Should the ability automatically commit cooldown when the duration effect ends?
 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Duration", meta=(EditCondition="AbilityType==EGASCourseAbilityType::Duration", EditConditionHides))
	bool bAutoCommitCooldownOnDurationEnd;

	/**
 * @brief Should the ability automatically end when the duration effect ends?
 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Duration", meta=(EditCondition="AbilityType==EGASCourseAbilityType::Duration", EditConditionHides))
	bool bAutoEndAbilityOnDurationEnd;

	/**
	 * @brief Indicates whether the ability should be canceled upon reactivation.
	 *
	 * If this flag is set to true, the ability will be canceled if it is reactivated while it is still active. By default, this flag is true, meaning the ability will continue to run even
	 * if it is reactivated.
	 *
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Duration", meta=(EditCondition="AbilityType==EGASCourseAbilityType::Duration", EditConditionHides))
	bool bCancelAbilityOnReactivation;
	
private:
	
	FActiveGameplayEffectHandle DurationEffectHandle;

protected:

	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec& Spec, FGameplayAbilitySpec* AbilitySpec) const override;
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) override;
	//virtual void CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	//~End of UGameplayAbility interface

	virtual void OnPawnAvatarSet();

	/** Called when this ability is granted to the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();

	/** Called when this ability is removed from the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	void K2_OnAbilityRemoved();

	/** Called when the ability system is initialized with a pawn avatar. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
	void K2_OnPawnAvatarSet();

	UFUNCTION(BlueprintPure, Category = "GASCourse|Ability|Tags")
	void GetStackedAbilityDurationTags(FGameplayTagContainer& DurationTags) const;

	/**
	 * ApplyAbilityParamsFromSourceObject
	 *
	 * Copies and applies ability parameter values from the source object associated with the provided
	 * gameplay ability spec to this ability instance. Utilizes property reflection to transfer values
	 * from the source object to matching properties defined on this ability's class.
	 *
	 * @param Spec The ability spec containing the source object with parameter data to be applied.
	 */
	UFUNCTION()
	void ApplyAbilityParamsFromSourceObject(const FGameplayAbilitySpec& Spec);

	/**
	 * ConfigureAbilityFromGrantedSpec
	 *
	 * Configures the ability instance based on the provided gameplay ability specification.
	 *
	 * @param Spec The gameplay ability specification used to initialize and configure the ability.
	 */
	UFUNCTION()
	void ConfigureAbilityFromGrantedSpec(const FGameplayAbilitySpec& Spec);

	/**
	 * ConfigureAbilityDuration
	 *
	 * Configures duration-related behavior for the ability using the specified card ability configuration.
	 *
	 * @param CardAbilityConfig The configuration structure containing the duration effect and related settings.
	 */
	UFUNCTION()
	void ConfigureAbilityDuration(const FGrantedCardAbilityConfig& CardAbilityConfig);

protected:

	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Activation")
	EGASCourseAbilityActivationPolicy ActivationPolicy;

	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Activation")
	EGASCourseAbilityType AbilityType;

	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Activation")
	EGASCourseAbilitySlotType AbilitySlotType;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Icon")
	TSoftObjectPtr<UTexture2D> AbilityIcon;
	
	/**
	 * @brief Should the ability automatically commit when activated? If false, blueprint or child classes must call CommitAbility() manually.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability")
	bool bAutoCommitAbilityOnActivate;

	// Stacking Ability Parameters

	/**
	 * bHasAbilityStacks
	 *
	 * Indicates whether the ability can utilize stacking mechanics.
	 * If true, the ability supports stacks that can influence its behavior or effects.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Stacking")
	bool bHasAbilityStacks = false;

	/**
	 * MaxNumberOfStacks
	 *
	 * Represents the maximum number of stacks an ability can have when stacking mechanics are enabled.
	 * This value is scalable and influences stacking-related gameplay effects or behavior.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Stacking", meta=(EditCondition="bHasAbilityStacks"))
	FScalableFloat MaxNumberOfStacks;

	/**
	 * StackDurationEffect
	 *
	 * Represents a subclass of UGameplayEffect that defines the effect applied
	 * to extend or influence the duration of an ability when stacking mechanics
	 * are enabled. This effect is only applicable if bHasAbilityStacks is true.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Stacking", meta=(EditCondition="bHasAbilityStacks"))
	TSubclassOf<UGameplayEffect> StackDurationEffect = nullptr;

	/**
	 * MaxStackAttribute
	 *
	 * Represents the maximum stack attribute associated with an ability when stacking mechanics are enabled.
	 * This attribute determines the limit of stacks that can influence the ability's behavior or effects.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Stacking", meta=(EditCondition="bHasAbilityStacks"))
	FGameplayAttribute MaxStackAttribute;

	/**
	 * CurrentStackAttribute
	 *
	 * Represents the current stack attribute associated with an ability when stacking mechanics are enabled.
	 * This attribute tracks the active number or value of stacks influencing the ability's behavior or effects.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASCourse|Ability|Stacking", meta=(EditCondition="bHasAbilityStacks"))
	FGameplayAttribute CurrentStackAttribute;

	/**
	 * AbilityCost
	 *
	 * Represents the cost associated with activating the ability.
	 * This value is scalable and can be modified based on gameplay systems or specific conditions.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Costs)
	FScalableFloat AbilityCost;

	/**
	 * InvokeAbilityFailHapticFeedback
	 *
	 * Plays a haptic feedback effect on the player's controller when an ability activation fails,
	 * if the player is the local controller and the corresponding settings are configured.
	 */
	UFUNCTION()
	void InvokeAbilityFailHapticFeedback() const;

	/**
	 * CachedInputDirection
	 *
	 * Stores the last known input direction as a FVector.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="GASCourse|Ability|Input")
	FVector CachedInputDirection = FVector::ZeroVector;

	UFUNCTION()
	void OnHitApplied(const FHitContext& HitContext);

	UFUNCTION()
	void OnHitReceived(const FHitContext& HitContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Ability|Damage Pipeline")
	void OnHitApplied_Event(const FHitContext& HitContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "GASCourse|Ability|Damage Pipeline")
	void OnHitReceived_Event(const FHitContext& HitContext);
	
private:
	
	FDelegateHandle DurationEffectRemovedDelegateHandle;
	
};
