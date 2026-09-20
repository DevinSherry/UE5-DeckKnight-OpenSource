// GASC_DamagePipelineSubsystem.h
// Final unified subsystem for Hit/Damage/Healing events

#pragma once

#include "GameplayEffectTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "GASC_ResourcePipelineTypes.h"
#include "GASC_ResourcePipelineDelegates.h"
#include "GameplayEffect.h"
#include "GASC_ResourcePipelineSubsystem.generated.h"

class UGameplayEffectExecutionCalculation;


/* =======================================================================================
 *  Per-object dynamic delegates (BP)
 * ======================================================================================= */

USTRUCT()
struct FOnHitEventListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> ListenerActor;

	UPROPERTY()
	FOnHitApplied_Event Callback;

	bool operator==(const FOnHitEventListener& Other) const
	{
		return ListenerActor == Other.ListenerActor &&
			   Callback.GetFunctionName() == Other.Callback.GetFunctionName();
	}
};

USTRUCT()
struct FOnDamageEventListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> ListenerActor;

	UPROPERTY()
	FOnDamageApplied_Event Callback;

	bool operator==(const FOnDamageEventListener& Other) const
	{
		return ListenerActor == Other.ListenerActor &&
			   Callback.GetFunctionName() == Other.Callback.GetFunctionName();
	}
};

USTRUCT()
struct FOnHealingEventListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> ListenerActor;

	UPROPERTY()
	FOnHealingApplied_Event Callback;

	bool operator==(const FOnHealingEventListener& Other) const
	{
		return ListenerActor == Other.ListenerActor &&
			   Callback.GetFunctionName() == Other.Callback.GetFunctionName();
	}
};

/* =======================================================================================
 *  SUBSYSTEM
 * ======================================================================================= */

UCLASS()
class GASCOURSE_API UGASC_ResourcePipelineSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/* ---------------------------------------------------------------------------------------
	 *  Lifecycle
	 * --------------------------------------------------------------------------------------- */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PostInitialize() override;

	/* ---------------------------------------------------------------------------------------
	 *  Blueprint global multicast convenience events (UI/HUD)
	 * --------------------------------------------------------------------------------------- */

	// Hit
	UPROPERTY(BlueprintAssignable)
	FOnHitApplied_BP OnHitApplied_BP;

	UPROPERTY(BlueprintAssignable)
	FOnHitReceived_BP OnHitReceived_BP;

	// Damage
	UPROPERTY(BlueprintAssignable)
	FOnDamageApplied_BP OnDamageApplied_BP;

	UPROPERTY(BlueprintAssignable)
	FOnDamageReceived_BP OnDamageReceived_BP;

	// Healing
	UPROPERTY(BlueprintAssignable)
	FOnHealingApplied_BP OnHealingApplied_BP;

	UPROPERTY(BlueprintAssignable)
	FOnHealingReceived_BP OnHealingReceived_BP;

	// Unified: fires for EVERY resource modification regardless of attribute (UI/HUD wide listeners)
	UPROPERTY(BlueprintAssignable)
	FOnResourceModified_BP OnAnyResourceModified_BP;

	/* ---------------------------------------------------------------------------------------
	 *  Native fast-path listeners (C++ only)
	 * --------------------------------------------------------------------------------------- */

	// Hit
	void RegisterNativeHitAppliedListener(UObject* Listener, FOnHitAppliedNative&& Callback);
	void RegisterNativeHitReceivedListener(UObject* Listener, FOnHitReceivedNative&& Callback);
	void UnregisterNativeHitListener(UObject* Listener);

	// Old Damage Pipeline Implementation
	// Damage
	//void RegisterNativeDamageAppliedListener(UObject* Listener, FOnDamageAppliedNative&& Callback);
	//void RegisterNativeDamageReceivedListener(UObject* Listener, FOnDamageReceivedNative&& Callback);
	//void UnregisterNativeDamageListener(UObject* Listener);

	// Old Damage Pipeline Implementation
	// Healing
	//void RegisterNativeHealingAppliedListener(UObject* Listener, FOnHealingAppliedNative&& Callback);
	//void RegisterNativeHealingReceivedListener(UObject* Listener, FOnHealingReceivedNative&& Callback);
	//void UnregisterNativeHealingListener(UObject* Listener);

	// Old Damage Pipeline Implementation
	// Backward-compatibility wrappers (old healing API)
	//void AddHealEventListener(AActor* ListenerActor, FOnHealingReceivedNative&& Delegate);
	//void RemoveHealingListener(AActor* ListenerActor);

	/* ---------------------------------------------------------------------------------------
	 *  UNIFIED attribute-keyed listener API (any attribute, Applied/Received)
	 * --------------------------------------------------------------------------------------- */

	// Native fast-path registration (C++). Listener binds one native callback for a given
	// attribute + direction. Re-registering the same listener/direction replaces the callback.
	void RegisterResourceListener(UObject* Listener, const FGameplayAttribute& Attribute,
		EResourceModificationEventDirection Direction, FOnResourceModifiedNative Callback);

	// Blueprint dynamic registration for a given attribute + direction.
	UFUNCTION(BlueprintCallable, Category = "Resource Pipeline")
	void RegisterResourceListener_BP(UObject* Listener, FGameplayAttribute Attribute,
		EResourceModificationEventDirection Direction, FOnResourceModified_Event Callback);

	// Removes every registration for a listener across all attributes/directions.
	UFUNCTION(BlueprintCallable, Category = "Resource Pipeline")
	void UnregisterAllForListener(UObject* Listener);

	// Single generalized broadcast entry point. Routes by Context.AffectedAttribute and also
	// fires the legacy Blueprint delegates below for backward compatibility.
	void BroadcastResourceModification(const FResourceModificationContext& Context);

	/* ---------------------------------------------------------------------------------------
	 *  Internal broadcast entry points (called from AttributeSets / pipeline)
	 * --------------------------------------------------------------------------------------- */

	// Hit
	void Internal_BroadcastHitApplied(const FHitContext& Context);
	void Internal_BroadcastHitReceived(const FHitContext& Context);

	// Damage
	void Internal_BroadcastDamageApplied(const FResourceModificationContext& Context);
	void Internal_BroadcastDamageReceived(const FResourceModificationContext& Context);

	// Healing
	void Internal_BroadcastHealingApplied(const FResourceModificationContext& Context);
	void Internal_BroadcastHealingReceived(const FResourceModificationContext& Context);

	/* ---------------------------------------------------------------------------------------
	 *  High-level HIT interface
	 * --------------------------------------------------------------------------------------- */

	UFUNCTION()
	bool OnHitEvent(const FHitContext& HitContext);

	UFUNCTION(BlueprintCallable)
	bool SendHitRequest(const FHitContext& HitContext);

	/* ---------------------------------------------------------------------------------------
	 *  Dynamic listener registration (Blueprint)
	 * --------------------------------------------------------------------------------------- */

	UFUNCTION()
	void RegisterActorOnHit(UObject* Listener, EHitEventType EventType, FOnHitApplied_Event Callback);

	UFUNCTION()
	void UnRegisterActorOnHit(UObject* Listener, EHitEventType EventType, FOnHitApplied_Event Callback);

	UFUNCTION()
	void RegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType EventType, FOnDamageApplied_Event Callback);

	UFUNCTION()
	void UnRegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType EventType, FOnDamageApplied_Event Callback);

	UFUNCTION()
	void RegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType EventType, FOnHealingApplied_Event Callback);

	UFUNCTION()
	void UnRegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType EventType, FOnHealingApplied_Event Callback);

	/* ---------------------------------------------------------------------------------------
	 *  Legacy forwarder function names (kept, but now just call broadcast)
	 * --------------------------------------------------------------------------------------- */

	UFUNCTION()
	void Internal_ForwardOnHitApplied(const FHitContext& Context);

	UFUNCTION()
	void Internal_ForwardOnHitReceived(const FHitContext& Context);

	UFUNCTION()
	void Internal_ForwardOnDamageApplied(const FResourceModificationContext& Context);

	UFUNCTION()
	void Internal_ForwardOnDamageReceived(const FResourceModificationContext& Context);

	UFUNCTION()
	void Internal_ForwardOnHealingApplied(const FResourceModificationContext& Context);

	UFUNCTION()
	void Internal_ForwardOnHealingReceived(const FResourceModificationContext& Context);

	/* ---------------------------------------------------------------------------------------
	 *  Unregister helpers (cleanup)
	 * --------------------------------------------------------------------------------------- */

	UFUNCTION()
	void UnregisterOnHitEventListener(UObject* Listener);

	UFUNCTION()
	void UnregisterOnDamageEventListener(UObject* Listener);

	UFUNCTION()
	void UnregisterOnHealingEventListener(UObject* Listener);

private:

	/* ---------------------------------------------------------------------------------------
	 *  DYNAMIC LISTENER ARRAYS
	 * --------------------------------------------------------------------------------------- */

	UPROPERTY()
	TArray<FOnHitEventListener> OnHitAppliedListeners;

	UPROPERTY()
	TArray<FOnHitEventListener> OnHitReceivedListeners;

	UPROPERTY()
	TArray<FOnDamageEventListener> OnDamageAppliedListeners;

	UPROPERTY()
	TArray<FOnDamageEventListener> OnDamageReceivedListeners;

	UPROPERTY()
	TArray<FOnHealingEventListener> OnHealingAppliedListeners;

	UPROPERTY()
	TArray<FOnHealingEventListener> OnHealingReceivedListeners;

	/* ---------------------------------------------------------------------------------------
	 *  NATIVE LISTENER ARRAYS
	 * --------------------------------------------------------------------------------------- */

	UPROPERTY()
	TArray<FNativeHitListener> NativeHitListeners;

	// Old Damage Pipeline Implementation
	//UPROPERTY()
	//TArray<FNativeDamageListener> NativeDamageListeners;

	// Old Damage Pipeline Implementation
	//UPROPERTY()
	//TArray<FNativeHealingListener> NativeHealingListeners;

	/* ---------------------------------------------------------------------------------------
	 *  UNIFIED attribute-keyed listener storage (replaces the per-family native arrays)
	 * --------------------------------------------------------------------------------------- */
	UPROPERTY()
	TMap<FGameplayAttribute, FAttributeListenerBucket> ResourceModificationListeners;

	/* ---------------------------------------------------------------------------------------
	 *  Logging & GameplayEffect helpers
	 * --------------------------------------------------------------------------------------- */
/*
	UPROPERTY()
	TArray<FHitLogEntry> HitLogEntries;
	*/

	UPROPERTY()
	TSubclassOf<UGameplayEffectExecutionCalculation> DamageCalculationClass;

public:

	/* ---------------------------------------------------------------------------------------
	 *  APPLY FUNCTIONS (unchanged)
	 * --------------------------------------------------------------------------------------- */

	FGameplayEffectSpecHandle ConstructDamageEffectSpecHandle(TWeakObjectPtr<AActor> Instigator, EGameplayEffectDurationType DurationType, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);
	FGameplayEffectSpecHandle ConstructHealingEffectSpecHandle(TWeakObjectPtr<AActor> Instigator, EGameplayEffectDurationType DurationType, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);

	UFUNCTION()
	bool ApplyDamageToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Damage, const FDamagePipelineContext& DamageContext);

	UFUNCTION()
	bool ApplyHealToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Heal, const FDamagePipelineContext& HealContext);

	bool ApplyDamageToTarget_Internal(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Damage, const FDamagePipelineContext& DamageContext, FGameplayEffectSpecHandle DamageSpecHandle);

	bool ApplyHealToTarget_Internal(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Heal, const FDamagePipelineContext& HealContext, FGameplayEffectSpecHandle HealSpecHandle);

	UFUNCTION()
	bool ApplyDamageToTargetDataHandle(const FGameplayAbilityTargetDataHandle& TargetHandle, TWeakObjectPtr<AActor> Instigator, float Damage, FDamagePipelineContext DamageContext);

	UFUNCTION()
	bool ApplyHealToTargetDataHandle(const FGameplayAbilityTargetDataHandle& TargetHandle, TWeakObjectPtr<AActor> Instigator, float Heal, FDamagePipelineContext HealContext);

	UFUNCTION()
	bool ApplyDamageOverTimeToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Damage, const FDamagePipelineContext& DamageContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);

	UFUNCTION()
	bool ApplyHealOverTimeToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Heal, const FDamagePipelineContext& HealContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);

	UFUNCTION()
	bool ApplyPhysicalDamageToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Damage, const FHitResult& HitResult, FDamagePipelineContext& DamageContext);

	UFUNCTION()
	bool ApplyFireDamageToTarget(TWeakObjectPtr<AActor> Target, TWeakObjectPtr<AActor> Instigator, float Damage, const FHitResult& HitResult, FDamagePipelineContext& DamageContext, bool bApplyBurnStack = true);
	
	//TODO
	//WeaponMana
};
