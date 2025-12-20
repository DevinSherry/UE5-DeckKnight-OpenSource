// GASC_DamagePipelineTypes.h
// Clean version – contains ONLY types, contexts, enums

#pragma once

#include "GameplayTagContainer.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "GASC_DamagePipelineTypes.generated.h"

// Log
DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_DamagePipelineSubsystem, Log, All);

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
enum EGASC_DamagePipelineType
{
	Damage  UMETA(DisplayName="Damage"),
	Healing UMETA(DisplayName="Healing")
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
struct FDamageModificationContext
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category="Damage Pipeline Context")
	FHitContext HitContext;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	TEnumAsByte<EGASC_DamagePipelineType> DamagePipelineType = EGASC_DamagePipelineType::Damage;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	FGameplayTag DamageType;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	float NewValue = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	float DeltaValue = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	bool bCriticalModification = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	bool bDamageModificationKilled = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	bool bModificationOverTime = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage Pipeline Context")
	bool bDamageResisted = false;
	
	FDamageModificationContext() = default;
};

/**
 * Effect-side context for applying damage/heal.
 */
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
struct FDamageLogEntry
{
	GENERATED_BODY()

	uint32 DamageTargetID     = 0;
	uint32 DamageInstigatorID = 0;
	uint32 OptionalObjectID   = 0;

	uint32 DamageID          = 0;
	float  DamageTimeStamp   = 0.0f;
	FHitResult HitResult;

	FString HitTargetName;
	FString HitInstigatorName;
	FString OptionalSourceObjectName;

	FGameplayTagContainer HitTargetTagsContainer;
	FGameplayTagContainer HitInstigatorTagsContainer;
	FGameplayTagContainer HitContextTagsContainer;
	
	TMap<FString, float> Attributes;
	float BaseDamageValue = 0.0f;
	float ModifiedDamageValue = 0.0f;
	float FinalDamageValue = 0.0f;
	
	bool bIsDamageEffect = false;
	bool bIsCriticalHit = false;
	bool bIsOverTimeEffect = false;
	bool bIsSimulatedDamage = false;
	bool bDamageResisted = false;

	FDamageLogEntry() = default;
};
