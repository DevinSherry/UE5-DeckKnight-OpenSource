// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineTypes.h"
#include "GASC_DamagePipelineStatics.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_DamagePipelineStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	/** ==== FHitContext helpers ==== */

	UFUNCTION(BlueprintPure, Category="Damage|HitContext")
	static FGameplayTagContainer GetHitTargetTags(const FHitContext& Context);

	UFUNCTION(BlueprintPure, Category="Damage|HitContext")
	static FGameplayTagContainer GetHitInstigatorTags(const FHitContext& Context);

	UFUNCTION(BlueprintPure, Category="Damage|HitContext")
	static FGameplayTagContainer GetHitContextTags(const FHitContext& Context);

	UFUNCTION(BlueprintPure, Category="Damage|HitContext")
	static FHitResult GetHitResultCopy(const FHitContext& Context);
	
	/** ==== FDamageModificationContext helpers ==== */

	UFUNCTION(BlueprintPure, Category="Damage|Context")
	static FHitContext GetHitContextCopy(const FDamageModificationContext& DamageContext);

	UFUNCTION(BlueprintPure, Category="Damage|Context")
	static FGameplayTagContainer GetHitTargetTagsFromDamageContext(const FDamageModificationContext& DamageContext);

	UFUNCTION(BlueprintPure, Category="Damage|Context")
	static FGameplayTagContainer GetHitInstigatorTagsFromDamageContext(const FDamageModificationContext& DamageContext);

	UFUNCTION(BlueprintPure, Category="Damage|Context")
	static FGameplayTagContainer GetHitContextTagsFromDamageContext(const FDamageModificationContext& DamageContext);

	UFUNCTION(BlueprintPure, Category="Damage|Context")
	static FHitResult GetHitResultCopyFromDamageContext(const FDamageModificationContext& DamageContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool RegisterActorOnHit(AActor* Listener, EHitEventType HitEvent , FOnHitApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool UnRegisterActorOnHit(AActor* Listener, EHitEventType HitEvent , FOnHitApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool RegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent , FOnDamageApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool UnRegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent , FOnDamageApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool RegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent , FOnHealingApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool UnRegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent , FOnHealingApplied_Event Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FDamagePipelineContext& DamageContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyHealToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyDamageToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Damage,FDamagePipelineContext DamageContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyHealToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Heal, FDamagePipelineContext HealContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyDamageOverTimeToTarget(AActor* Target, AActor* Instigator, float Damage, const FDamagePipelineContext& DamageContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);
	
	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	static bool ApplyHealOverTimeToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext);
	
	UFUNCTION(BlueprintCallable, meta=(Hidepin = "DamageContext"), Category = "Damage Pipeline")
	static bool ApplyPhysicalDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult, FDamagePipelineContext& DamageContext);
	
	UFUNCTION(BlueprintCallable, meta=(Hidepin = "DamageContext"), BlueprintAuthorityOnly, Category = "GASCourse|AbilitySystem|Damage")
	static bool ApplyFireDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult, FDamagePipelineContext& DamageContext, bool bApplyBurnStack = true);
};
