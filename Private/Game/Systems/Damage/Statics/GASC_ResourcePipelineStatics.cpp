// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/Statics/GASC_ResourcePipelineStatics.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitTargetTags(const FHitContext& Context)
{
	return Context.HitTargetTagsContainer ? *Context.HitTargetTagsContainer : FGameplayTagContainer();
}

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitInstigatorTags(const FHitContext& Context)
{
	return Context.HitInstigatorTagsContainer ? *Context.HitInstigatorTagsContainer : FGameplayTagContainer();
}

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitContextTags(const FHitContext& Context)
{
	return Context.HitContextTagsContainer ? *Context.HitContextTagsContainer : FGameplayTagContainer();
}

FHitResult UGASC_ResourcePipelineStatics::GetHitResultCopy(const FHitContext& Context)
{
	return Context.HitResult.bBlockingHit ? Context.HitResult : FHitResult();
}

/* ----- FDamageModificationContext helpers ----- */

FHitContext UGASC_ResourcePipelineStatics::GetHitContextCopy(const FResourceModificationContext& ResourceModContext)
{
	return ResourceModContext.HitContext;
}

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitTargetTagsFromDamageContext(const FResourceModificationContext& ResourceModContext)
{
	return (ResourceModContext.HitContext.HitTargetTagsContainer)
		? *ResourceModContext.HitContext.HitTargetTagsContainer
		: FGameplayTagContainer();
}

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitInstigatorTagsFromDamageContext(const FResourceModificationContext& ResourceModContext)
{
	return (ResourceModContext.HitContext.HitInstigatorTagsContainer)
		? *ResourceModContext.HitContext.HitInstigatorTagsContainer
		: FGameplayTagContainer();
}

FGameplayTagContainer UGASC_ResourcePipelineStatics::GetHitContextTagsFromDamageContext(const FResourceModificationContext& ResourceModContext)
{
	return (ResourceModContext.HitContext.HitContextTagsContainer)
		? *ResourceModContext.HitContext.HitContextTagsContainer
		: FGameplayTagContainer();
}

FHitResult UGASC_ResourcePipelineStatics::GetHitResultCopyFromDamageContext(const FResourceModificationContext& ResourceModContext)
{
	return (ResourceModContext.HitContext.HitResult.bBlockingHit)
		? ResourceModContext.HitContext.HitResult
		: FHitResult();
}

bool UGASC_ResourcePipelineStatics::RegisterActorOnHit(AActor* Listener, EHitEventType HitEvent, FOnHitApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnHit(Listener, HitEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::UnRegisterActorOnHit(AActor* Listener, EHitEventType HitEvent, FOnHitApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnHit(Listener, HitEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::RegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent, FOnDamageApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnDamageEvent(Listener, DamageEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::UnRegisterActorOnDamageEvent(UObject* Listener, EOnDamageEventType DamageEvent, FOnDamageApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnDamageEvent(Listener, DamageEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::RegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent, FOnHealingApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->RegisterActorOnHealingEvent(Listener, HealingEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::UnRegisterActorOnHealingEvent(UObject* Listener, EOnHealingEventType HealingEvent, FOnHealingApplied_Event Callback)
{
	if (!Listener)
	{
		return false;
	}
	if (UWorld* World = Listener->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			DamagePipelineSubsystem->UnRegisterActorOnHealingEvent(Listener, HealingEvent, Callback);
			return true;
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
	__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FDamagePipelineContext& DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageToTarget(Target, Instigator, Damage, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyHealToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealToTarget(Target, Instigator, Heal, HealContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyDamageToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Damage,
                                                                FDamagePipelineContext DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target Handle",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageToTargetDataHandle(TargetHandle, Instigator, Damage, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyHealToTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, AActor* Instigator, float Heal,
	FDamagePipelineContext HealContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target Handle",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealToTargetDataHandle(TargetHandle, Instigator, Heal, HealContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyDamageOverTimeToTarget(AActor* Target, AActor* Instigator, float Damage,
                                                              const FDamagePipelineContext& DamageContext, const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyDamageOverTimeToTarget(Target, Instigator, Damage, DamageContext, EffectOverTimeContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyHealOverTimeToTarget(AActor* Target, AActor* Instigator, float Heal, const FDamagePipelineContext& HealContext,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Healing will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyHealOverTimeToTarget(Target, Instigator, Heal, HealContext, EffectOverTimeContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyPhysicalDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult,
                                                              FDamagePipelineContext& DamageContext)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyPhysicalDamageToTarget(Target, Instigator, Damage, HitResult, DamageContext);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}

bool UGASC_ResourcePipelineStatics::ApplyFireDamageToTarget(AActor* Target, AActor* Instigator, float Damage, const FHitResult& HitResult,
	FDamagePipelineContext& DamageContext, bool bApplyBurnStack)
{
	if (!Instigator)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Instigator is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (!Target)
	{
		UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning,"Target is invalid in function {0}. Damage will not be applied to Target",
			__FUNCTION__);
		return false;
	}
	if (UWorld* World = Instigator->GetWorld())
	{
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = World->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			return DamagePipelineSubsystem->ApplyFireDamageToTarget(Target, Instigator, Damage, HitResult, DamageContext, bApplyBurnStack);
		}
	}
	
	UE_LOGFMT(LOG_GASC_ResourcePipelineSubsystem, Warning, "Unable to obtain World in function {0}; therefore unable to obtain damage pipeline subsystem.", 
		__FUNCTION__);
	return false;
}
