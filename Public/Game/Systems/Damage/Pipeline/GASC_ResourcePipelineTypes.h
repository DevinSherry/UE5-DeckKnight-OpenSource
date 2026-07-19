// GASC_ResourceipelineTypes.h
// Clean version – contains ONLY types, contexts, enums

#pragma once

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "GASC_ResourcePipelineTypes.generated.h"

// Log
DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_ResourcePipelineSubsystem, Log, All);

/**
 * @enum EHitEventType
 */
UENUM(BlueprintType)
enum EHitEventType
{
	OnHitReceived UMETA(DisplayName="On Hit Received"),
	OnHitApplied  UMETA(DisplayName="On Hit Applied")
};

UENUM(BlueprintType)
enum class EResourceModificationEventDirection: uint8
{
	Received UMETA(DisplayName="Received"),
	Applied  UMETA(DisplayName="Applied")
};

UENUM(BlueprintType)
enum EOnDamageEventType
{
	OnDamageReceived UMETA(DisplayName="On Damage Received"),
	OnDamageApplied  UMETA(DisplayName="On Damage Applied")
};

UENUM(BlueprintType)
enum EOnHealingEventType
{
	OnHealingReceived UMETA(DisplayName="On Healing Received"),
	OnHealingApplied  UMETA(DisplayName="On Healing Applied")
};

UENUM(BlueprintType)
enum EGASC_ResourcePipelineType
{
	Damage		UMETA(DisplayName="Damage"),
	Healing		UMETA(DisplayName="Healing"),
	WeaponMana	UMETA(DisplayName="Weapon Mana"),
};

/**
 * Lightweight hit context – no heavy copies.
 */
USTRUCT(BlueprintType)
struct FHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<AActor> HitTarget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<AActor> HitInstigator = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	TWeakObjectPtr<AActor> OptionalSourceObject = nullptr;

	// Non-UPROPERTY (internal only)
	const FGameplayTagContainer* HitTargetTagsContainer     = nullptr;
	const FGameplayTagContainer* HitInstigatorTagsContainer = nullptr;
	const FGameplayTagContainer* HitContextTagsContainer    = nullptr;

	/** SAFELY STORED HitResult */
	UPROPERTY(BlueprintReadOnly, Category="Hit Context")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Context")
	float HitTimeStamp = 0.0f;

	FHitContext() = default;
};

/**
 * Damage + Healing modification context.
 */
USTRUCT(BlueprintType)
struct FResourceModificationContext
{
	GENERATED_BODY()
	
	// NEW: the attribute that actually changed (Health, Mana, WeaponMana, CardEnergy…)
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttribute AffectedAttribute;
	
	UPROPERTY(BlueprintReadOnly, Category="Resource Pipeline Context")
	FHitContext HitContext;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	TEnumAsByte<EGASC_ResourcePipelineType> ResourcePipelineType = EGASC_ResourcePipelineType::Damage;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	FGameplayTag DamageType;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	float NewValue = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	float DeltaValue = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	bool bCriticalModification = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	bool bResourceModificationKilled = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	bool bModificationOverTime = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	bool bResourceResisted = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource Pipeline Context")
	bool bLifeSteal = false;
	
	FResourceModificationContext() = default;
};

/**
 * Effect-side context for applying damage/heal.
 */

//TODO Figure out what should change here
USTRUCT(BlueprintType)
struct FDamagePipelineContext
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Damage.Type"))
	FGameplayTag DamageType = DamageType_Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer GrantedTags;
};

/**
 * Over-time effect tuning.
 */
USTRUCT(BlueprintType)
struct FDamagePipelineEffectOverTimeContext
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectPeriod = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectDuration = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bApplyValueOverTotalDuration = false;
};

/**
 * Logging entry – used for debug UI / pipeline logging.
 */
USTRUCT(BlueprintType)
struct FResourceLogEntry
{
	GENERATED_BODY()

	uint32 ResourceTargetID     = 0;
	uint32 ResourceInstigatorID = 0;
	uint32 OptionalObjectID   = 0;

	uint32 ResourceID          = 0;
	float  ResourceTimeStamp   = 0.0f;
	FHitResult HitResult;

	FString HitTargetName;
	FString HitInstigatorName;
	FString OptionalSourceObjectName;

	FGameplayTagContainer HitTargetTagsContainer;
	FGameplayTagContainer HitInstigatorTagsContainer;
	FGameplayTagContainer HitContextTagsContainer;
	
	TMap<FString, float> Attributes;
	float BaseResourceValue = 0.0f;
	float ModifiedResourceValue = 0.0f;
	float FinalResourceValue = 0.0f;
	
	bool bIsDamageEffect = false;
	bool bIsCriticalHit = false;
	bool bIsOverTimeEffect = false;
	bool bIsSimulatedResourceMod = false;
	bool bDamageResisted = false;
	bool bLifeSteal = false;

	FResourceLogEntry() = default;
};
