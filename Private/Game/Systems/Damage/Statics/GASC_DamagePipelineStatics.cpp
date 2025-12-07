// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/Statics/GASC_DamagePipelineStatics.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitTargetTags(const FHitContext& Context)
{
	return Context.HitTargetTagsContainer ? *Context.HitTargetTagsContainer : FGameplayTagContainer();
}

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitInstigatorTags(const FHitContext& Context)
{
	return Context.HitInstigatorTagsContainer ? *Context.HitInstigatorTagsContainer : FGameplayTagContainer();
}

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitContextTags(const FHitContext& Context)
{
	return Context.HitContextTagsContainer ? *Context.HitContextTagsContainer : FGameplayTagContainer();
}

FHitResult UGASC_DamagePipelineStatics::GetHitResultCopy(const FHitContext& Context)
{
	return Context.HitResult.bBlockingHit ? Context.HitResult : FHitResult();
}

/* ----- FDamageModificationContext helpers ----- */

FHitContext UGASC_DamagePipelineStatics::GetHitContextCopy(const FDamageModificationContext& DamageContext)
{
	return DamageContext.HitContext;
}

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitTargetTagsFromDamageContext(const FDamageModificationContext& DamageContext)
{
	return (DamageContext.HitContext.HitTargetTagsContainer)
		? *DamageContext.HitContext.HitTargetTagsContainer
		: FGameplayTagContainer();
}

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitInstigatorTagsFromDamageContext(const FDamageModificationContext& DamageContext)
{
	return (DamageContext.HitContext.HitInstigatorTagsContainer)
		? *DamageContext.HitContext.HitInstigatorTagsContainer
		: FGameplayTagContainer();
}

FGameplayTagContainer UGASC_DamagePipelineStatics::GetHitContextTagsFromDamageContext(const FDamageModificationContext& DamageContext)
{
	return (DamageContext.HitContext.HitContextTagsContainer)
		? *DamageContext.HitContext.HitContextTagsContainer
		: FGameplayTagContainer();
}

FHitResult UGASC_DamagePipelineStatics::GetHitResultCopyFromDamageContext(const FDamageModificationContext& DamageContext)
{
	return (DamageContext.HitContext.HitResult.bBlockingHit)
		? DamageContext.HitContext.HitResult
		: FHitResult();
}

bool UGASC_DamagePipelineStatics::RegisterActorOnHit(AActor* Listener, EHitEventType HitEvent, FOnHitApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnHit(Listener, HitEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::UnRegisterActorOnHit(AActor* Listener, EHitEventType HitEvent, FOnHitApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnHit(Listener, HitEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::RegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent, FOnDamageApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnDamageEvent(Listener, DamageEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::UnRegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent, FOnDamageApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnDamageEvent(Listener, DamageEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::RegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent, FOnHealingApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnHealingEvent(Listener, HealingEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::UnRegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent, FOnHealingApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnHealingEvent(Listener, HealingEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FDamagePipelineContext& DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageToTarget(Target, Instigator, Damage, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyHealToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealToTarget(Target, Instigator, Heal, HealContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyDamageToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Damage,
                                                                FDamagePipelineContext DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target Handle",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageToTargetDataHandle(TargetHandle, Instigator, Damage, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyHealToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Heal,
	FDamagePipelineContext HealContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target Handle",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealToTargetDataHandle(TargetHandle, Instigator, Heal, HealContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyDamageOverTimeToTarget(AActor* Target, AActor* Instigator, float Damage,
                                                              const FDamagePipelineContext& DamageContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageOverTimeToTarget(Target, Instigator, Damage, DamageContext, EffectOverTimeContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyHealOverTimeToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealOverTimeToTarget(Target, Instigator, Heal, HealContext, EffectOverTimeContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyPhysicalDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult,
                                                              FDamagePipelineContext& DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyPhysicalDamageToTarget(Target, Instigator, Damage, HitResult, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_DamagePipelineStatics::ApplyFireDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult,
	FDamagePipelineContext& DamageContext, bool bApplyBurnStack)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyFireDamageToTarget(Target, Instigator, Damage, HitResult, DamageContext, bApplyBurnStack);
		}
	}
	
	UE_LOGFMT(LOG_GASC_DamagePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}
