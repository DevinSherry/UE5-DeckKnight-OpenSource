// GASC_DamagePipelineSubsystem.cpp

#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Game/Character/Player/GASCoursePlayerState.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Game/Systems/Damage/Debug/DamagePipelineDebugSubsystem.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "AbilitySystemGlobals.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/GASC_GameplayEffectContextTypes.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Damage/GASC_DamageGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Damage/GASC_DamageOverTimeGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Healing/GASC_HealingGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GameplayEffect/Healing/GASC_HealingOverTimeGameplayEffect.h"

void UGASC_DamagePipelineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGASC_DamagePipelineSubsystem::Deinitialize()
{
	// Clear all dynamic listener arrays
	OnHitAppliedListeners.Empty();
	OnHitReceivedListeners.Empty();
	OnDamageAppliedListeners.Empty();
	OnDamageReceivedListeners.Empty();
	OnHealingAppliedListeners.Empty();
	OnHealingReceivedListeners.Empty();

	// Clear native listeners
	NativeHitListeners.Empty();
	NativeDamageListeners.Empty();
	NativeHealingListeners.Empty();

	// Clear global BP delegates
	OnHitApplied_BP.Clear();
	OnHitReceived_BP.Clear();
	OnDamageApplied_BP.Clear();
	OnDamageReceived_BP.Clear();
	OnHealingApplied_BP.Clear();
	OnHealingReceived_BP.Clear();

	Super::Deinitialize();
}

void UGASC_DamagePipelineSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

/* ===========================================================================================================
 *                                      NATIVE HIT LISTENERS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::RegisterNativeHitAppliedListener(
	UObject* Listener,
	FOnHitAppliedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
	{
		return;
	}

	for (FNativeHitListener& Entry : NativeHitListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnApplied = MoveTemp(Callback);
			return;
		}
	}

	FNativeHitListener& NewEntry = NativeHitListeners.AddDefaulted_GetRef();
	NewEntry.Listener  = Listener;
	NewEntry.OnApplied = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::RegisterNativeHitReceivedListener(
	UObject* Listener,
	FOnHitReceivedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
	{
		return;
	}

	for (FNativeHitListener& Entry : NativeHitListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnReceived = MoveTemp(Callback);
			return;
		}
	}

	FNativeHitListener& NewEntry = NativeHitListeners.AddDefaulted_GetRef();
	NewEntry.Listener   = Listener;
	NewEntry.OnReceived = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::UnregisterNativeHitListener(UObject* Listener)
{
	if (!IsValid(Listener))
	{
		return;
	}

	NativeHitListeners.RemoveAll(
		[Listener](const FNativeHitListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

/* ===========================================================================================================
 *                                      NATIVE DAMAGE LISTENERS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::RegisterNativeDamageAppliedListener(
	UObject* Listener,
	FOnDamageAppliedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
	{
		return;
	}

	for (FNativeDamageListener& Entry : NativeDamageListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnApplied = MoveTemp(Callback);
			return;
		}
	}

	FNativeDamageListener& NewEntry = NativeDamageListeners.AddDefaulted_GetRef();
	NewEntry.Listener  = Listener;
	NewEntry.OnApplied = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::RegisterNativeDamageReceivedListener(
	UObject* Listener,
	FOnDamageReceivedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
	{
		return;
	}

	for (FNativeDamageListener& Entry : NativeDamageListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnReceived = MoveTemp(Callback);
			return;
		}
	}

	FNativeDamageListener& NewEntry = NativeDamageListeners.AddDefaulted_GetRef();
	NewEntry.Listener   = Listener;
	NewEntry.OnReceived = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::UnregisterNativeDamageListener(UObject* Listener)
{
	if (!IsValid(Listener))
	{
		return;
	}

	NativeDamageListeners.RemoveAll(
		[Listener](const FNativeDamageListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

/* ===========================================================================================================
 *                                      NATIVE HEALING LISTENERS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::RegisterNativeHealingAppliedListener(
	UObject* Listener,
	FOnHealingAppliedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
		return;

	for (FNativeHealingListener& Entry : NativeHealingListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnApplied = MoveTemp(Callback);
			return;
		}
	}

	FNativeHealingListener& NewEntry = NativeHealingListeners.AddDefaulted_GetRef();
	NewEntry.Listener  = Listener;
	NewEntry.OnApplied = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::RegisterNativeHealingReceivedListener(
	UObject* Listener,
	FOnHealingReceivedNative&& Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
		return;

	for (FNativeHealingListener& Entry : NativeHealingListeners)
	{
		if (Entry.Listener.Get() == Listener)
		{
			Entry.OnReceived = MoveTemp(Callback);
			return;
		}
	}

	FNativeHealingListener& NewEntry = NativeHealingListeners.AddDefaulted_GetRef();
	NewEntry.Listener   = Listener;
	NewEntry.OnReceived = MoveTemp(Callback);
}

void UGASC_DamagePipelineSubsystem::UnregisterNativeHealingListener(UObject* Listener)
{
	if (!IsValid(Listener))
		return;

	NativeHealingListeners.RemoveAll(
		[Listener](const FNativeHealingListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

/* ------- Backward compatibility wrappers (old Add/RemoveHealEventListener) ------- */

void UGASC_DamagePipelineSubsystem::AddHealEventListener(
	AActor* ListenerActor,
	FOnHealingReceivedNative&& Delegate)
{
	RegisterNativeHealingReceivedListener(ListenerActor, MoveTemp(Delegate));
}

void UGASC_DamagePipelineSubsystem::RemoveHealingListener(AActor* ListenerActor)
{
	UnregisterNativeHealingListener(ListenerActor);
}

/* ===========================================================================================================
 *                                         INTERNAL BROADCAST ENTRY POINTS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::Internal_BroadcastHitApplied(const FHitContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnHitAppliedListeners.Num() - 1; i >= 0; --i)
	{
		FOnHitEventListener& L = OnHitAppliedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnHitAppliedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeHitListeners.Num() - 1; i >= 0; --i)
	{
		FNativeHitListener& Entry = NativeHitListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeHitListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnApplied.IsBound())
		{
			Entry.OnApplied.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnHitApplied_BP.Broadcast(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_BroadcastHitReceived(const FHitContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnHitReceivedListeners.Num() - 1; i >= 0; --i)
	{
		FOnHitEventListener& L = OnHitReceivedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnHitReceivedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeHitListeners.Num() - 1; i >= 0; --i)
	{
		FNativeHitListener& Entry = NativeHitListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeHitListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnReceived.IsBound())
		{
			Entry.OnReceived.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnHitReceived_BP.Broadcast(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_BroadcastDamageApplied(
	const FDamageModificationContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnDamageAppliedListeners.Num() - 1; i >= 0; --i)
	{
		FOnDamageEventListener& L = OnDamageAppliedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnDamageAppliedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeDamageListeners.Num() - 1; i >= 0; --i)
	{
		FNativeDamageListener& Entry = NativeDamageListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeDamageListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnApplied.IsBound())
		{
			Entry.OnApplied.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnDamageApplied_BP.Broadcast(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_BroadcastDamageReceived(
	const FDamageModificationContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnDamageReceivedListeners.Num() - 1; i >= 0; --i)
	{
		FOnDamageEventListener& L = OnDamageReceivedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnDamageReceivedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeDamageListeners.Num() - 1; i >= 0; --i)
	{
		FNativeDamageListener& Entry = NativeDamageListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeDamageListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnReceived.IsBound())
		{
			Entry.OnReceived.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnDamageReceived_BP.Broadcast(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_BroadcastHealingApplied(
	const FDamageModificationContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnHealingAppliedListeners.Num() - 1; i >= 0; --i)
	{
		FOnHealingEventListener& L = OnHealingAppliedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnHealingAppliedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeHealingListeners.Num() - 1; i >= 0; --i)
	{
		FNativeHealingListener& Entry = NativeHealingListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeHealingListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnApplied.IsBound())
		{
			Entry.OnApplied.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnHealingApplied_BP.Broadcast(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_BroadcastHealingReceived(
	const FDamageModificationContext& Context)
{
	// 1) Dynamic per-object listeners
	for (int32 i = OnHealingReceivedListeners.Num() - 1; i >= 0; --i)
	{
		FOnHealingEventListener& L = OnHealingReceivedListeners[i];

		if (UObject* Obj = L.ListenerActor.Get())
		{
			L.Callback.ExecuteIfBound(Context);
		}
		else
		{
			OnHealingReceivedListeners.RemoveAtSwap(i);
		}
	}

	// 2) Native listeners
	for (int32 i = NativeHealingListeners.Num() - 1; i >= 0; --i)
	{
		FNativeHealingListener& Entry = NativeHealingListeners[i];

		if (!Entry.Listener.IsValid())
		{
			NativeHealingListeners.RemoveAtSwap(i);
			continue;
		}

		if (Entry.OnReceived.IsBound())
		{
			Entry.OnReceived.Execute(Context);
		}
	}

	// 3) Global BP convenience event
	OnHealingReceived_BP.Broadcast(Context);
}

/* ===========================================================================================================
 *                              LEGACY FORWARDER NAMES (NOW WRAPPERS)
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnHitApplied(const FHitContext& Context)
{
	Internal_BroadcastHitApplied(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnHitReceived(const FHitContext& Context)
{
	Internal_BroadcastHitReceived(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnDamageApplied(
	const FDamageModificationContext& Context)
{
	Internal_BroadcastDamageApplied(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnDamageReceived(
	const FDamageModificationContext& Context)
{
	Internal_BroadcastDamageReceived(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnHealingApplied(
	const FDamageModificationContext& Context)
{
	Internal_BroadcastHealingApplied(Context);
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnHealingReceived(
	const FDamageModificationContext& Context)
{
	Internal_BroadcastHealingReceived(Context);
}

/* ===========================================================================================================
 *                                              HIT EVENTS
 * =========================================================================================================== */

bool UGASC_DamagePipelineSubsystem::OnHitEvent(const FHitContext& HitContext)
{
	if (!HitContext.HitTarget.IsValid())
	{
		UE_LOGFMT(LogTemp, Error, "Hit Target is invalid. OnHitEvent will not be broadcasted.");
		return false;
	}
	if (!HitContext.HitInstigator.IsValid())
	{
		UE_LOGFMT(LogTemp, Error, "Hit Instigator is invalid. OnHitEvent will not be broadcasted.");
		return false;
	}

	// Fire both sides through the unified pipeline
	Internal_BroadcastHitApplied(HitContext);
	Internal_BroadcastHitReceived(HitContext);

	return true;
}

bool UGASC_DamagePipelineSubsystem::SendHitRequest(const FHitContext& HitContext)
{
	return OnHitEvent(HitContext);
}

void UGASC_DamagePipelineSubsystem::RegisterActorOnHit(
	UObject* Listener,
	EHitEventType HitEvent,
	FOnHitApplied_Event Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
		return;

	FOnHitEventListener Entry;
	Entry.ListenerActor = Listener;
	Entry.Callback      = Callback;

	if (HitEvent == OnHitApplied)
	{
		OnHitAppliedListeners.Add(Entry);
	}
	else
	{
		OnHitReceivedListeners.Add(Entry);
	}
}

void UGASC_DamagePipelineSubsystem::UnRegisterActorOnHit(
	UObject* Listener,
	EHitEventType HitEvent,
	FOnHitApplied_Event Callback)
{
	if (!IsValid(Listener))
		return;

	auto Match = [&](const FOnHitEventListener& L)
	{
		return L.ListenerActor == Listener &&
			   L.Callback.GetFunctionName() == Callback.GetFunctionName();
	};

	if (HitEvent == EHitEventType::OnHitApplied)
	{
		OnHitAppliedListeners.RemoveAll(Match);
	}
	else
	{
		OnHitReceivedListeners.RemoveAll(Match);
	}
}

/* ===========================================================================================================
 *                                             DAMAGE EVENTS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::RegisterActorOnDamageEvent(
	UObject* Listener,
	EOnDamageEventType DamageEvent,
	FOnDamageApplied_Event Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
		return;

	FOnDamageEventListener Entry;
	Entry.ListenerActor = Listener;
	Entry.Callback      = Callback;

	if (DamageEvent == OnDamageApplied)
	{
		OnDamageAppliedListeners.Add(Entry);
	}
	else
	{
		OnDamageReceivedListeners.Add(Entry);
	}
}

void UGASC_DamagePipelineSubsystem::UnRegisterActorOnDamageEvent(
	UObject* Listener,
	EOnDamageEventType DamageEvent,
	FOnDamageApplied_Event Callback)
{
	if (!IsValid(Listener))
		return;

	auto Match = [&](const FOnDamageEventListener& L)
	{
		return L.ListenerActor == Listener &&
			   L.Callback.GetFunctionName() == Callback.GetFunctionName();
	};

	if (DamageEvent == OnDamageApplied)
	{
		OnDamageAppliedListeners.RemoveAll(Match);
	}
	else
	{
		OnDamageReceivedListeners.RemoveAll(Match);
	}
}

/* ===========================================================================================================
 *                                        HEALING EVENTS (DYNAMIC + NATIVE)
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::RegisterActorOnHealingEvent(
	UObject* Listener,
	EOnHealingEventType HealingEvent,
	FOnHealingApplied_Event Callback)
{
	if (!IsValid(Listener) || !Callback.IsBound())
		return;

	FOnHealingEventListener Entry;
	Entry.ListenerActor = Listener;
	Entry.Callback      = Callback;

	if (HealingEvent == OnHealingApplied)
	{
		OnHealingAppliedListeners.Add(Entry);
	}
	else  // OnHealingReceived
	{
		OnHealingReceivedListeners.Add(Entry);
	}
}

void UGASC_DamagePipelineSubsystem::UnRegisterActorOnHealingEvent(
	UObject* Listener,
	EOnHealingEventType HealingEvent,
	FOnHealingApplied_Event Callback)
{
	if (!IsValid(Listener))
		return;

	auto MatchDynamic = [&](const FOnHealingEventListener& L)
	{
		return L.ListenerActor.Get() == Listener &&
			   L.Callback.GetFunctionName() == Callback.GetFunctionName();
	};

	if (HealingEvent == EOnHealingEventType::OnHealingApplied)
	{
		OnHealingAppliedListeners.RemoveAll(MatchDynamic);
	}
	else // OnHealingReceived
	{
		OnHealingReceivedListeners.RemoveAll(MatchDynamic);
		UnregisterNativeHealingListener(Listener);
	}
}

/* ===========================================================================================================
 *                                              UNREGISTER HELPERS
 * =========================================================================================================== */

void UGASC_DamagePipelineSubsystem::UnregisterOnDamageEventListener(UObject* Listener)
{
	if (!IsValid(Listener))
		return;

	OnDamageAppliedListeners.RemoveAll(
		[&](const FOnDamageEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	OnDamageReceivedListeners.RemoveAll(
		[&](const FOnDamageEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	NativeDamageListeners.RemoveAll(
		[Listener](const FNativeDamageListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

void UGASC_DamagePipelineSubsystem::UnregisterOnHealingEventListener(UObject* Listener)
{
	if (!IsValid(Listener))
		return;

	OnHealingAppliedListeners.RemoveAll(
		[&](const FOnHealingEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	OnHealingReceivedListeners.RemoveAll(
		[&](const FOnHealingEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	NativeHealingListeners.RemoveAll(
		[Listener](const FNativeHealingListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

void UGASC_DamagePipelineSubsystem::UnregisterOnHitEventListener(UObject* Listener)
{
	if (!IsValid(Listener))
		return;

	OnHitAppliedListeners.RemoveAll(
		[&](const FOnHitEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	OnHitReceivedListeners.RemoveAll(
		[&](const FOnHitEventListener& L)
		{
			return L.ListenerActor == Listener;
		});

	NativeHitListeners.RemoveAll(
		[Listener](const FNativeHitListener& L)
		{
			return L.Listener.Get() == Listener;
		});
}

/* ===========================================================================================================
 *                               DAMAGE / HEAL APPLICATION (UNCHANGED LOGIC)
 * =========================================================================================================== */

FGameplayEffectSpecHandle UGASC_DamagePipelineSubsystem::ConstructDamageEffectSpecHandle(
	TWeakObjectPtr<AActor> Instigator,
	EGameplayEffectDurationType DurationType,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	AActor* InstigatorActor = Instigator.Get();

	if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(InstigatorActor))
	{
		InstigatorActor = PS->GetPawn();
	}

	AGASCourseCharacter* InstigatorChar = Cast<AGASCourseCharacter>(InstigatorActor);
	if (!InstigatorChar)
		return FGameplayEffectSpecHandle();

	UGASCourseAbilitySystemComponent* InstigatorASC =
		InstigatorChar->GetAbilitySystemComponent();

	if (!InstigatorASC)
		return FGameplayEffectSpecHandle();

	// Create context
	FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
	ContextHandle.AddInstigator(InstigatorChar, InstigatorChar);

	FGameplayEffectSpecHandle SpecHandle;

	if (DurationType == EGameplayEffectDurationType::Instant)
	{
		SpecHandle = InstigatorASC->MakeOutgoingSpec(
			UGASC_DamageGameplayEffect::StaticClass(),
			1.f,
			ContextHandle
		);
	}
	else
	{
		SpecHandle = InstigatorASC->MakeOutgoingSpec(
			UGASC_DamageOverTimeGameplayEffect::StaticClass(),
			1.f,
			ContextHandle
		);

		if (FGameplayEffectSpec* Spec = SpecHandle.Data.Get())
		{
			Spec->SetDuration(EffectOverTimeContext.EffectDuration, true);
			Spec->Period = EffectOverTimeContext.EffectPeriod;
		}
	}

	return SpecHandle;
}

FGameplayEffectSpecHandle UGASC_DamagePipelineSubsystem::ConstructHealingEffectSpecHandle(
	TWeakObjectPtr<AActor> Instigator,
	EGameplayEffectDurationType DurationType,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	AActor* InstigatorActor = Instigator.Get();

	if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(InstigatorActor))
	{
		InstigatorActor = PS->GetPawn();
	}

	AGASCourseCharacter* InstigatorChar = Cast<AGASCourseCharacter>(InstigatorActor);
	if (!InstigatorChar)
		return FGameplayEffectSpecHandle();

	UGASCourseAbilitySystemComponent* InstigatorASC =
		InstigatorChar->GetAbilitySystemComponent();

	if (!InstigatorASC)
		return FGameplayEffectSpecHandle();

	FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
	ContextHandle.AddInstigator(InstigatorChar, InstigatorChar);

	FGameplayEffectSpecHandle SpecHandle;

	if (DurationType == EGameplayEffectDurationType::Instant)
	{
		SpecHandle = InstigatorASC->MakeOutgoingSpec(
			UGASC_HealingGameplayEffect::StaticClass(),
			1.f,
			ContextHandle
		);
	}
	else
	{
		SpecHandle = InstigatorASC->MakeOutgoingSpec(
			UGASC_HealingOverTimeGameplayEffect::StaticClass(),
			1.f,
			ContextHandle
		);

		if (FGameplayEffectSpec* Spec = SpecHandle.Data.Get())
		{
			Spec->SetDuration(EffectOverTimeContext.EffectDuration, true);
			Spec->Period = EffectOverTimeContext.EffectPeriod;
		}
	}

	return SpecHandle;
}

bool UGASC_DamagePipelineSubsystem::ApplyDamageToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	const FDamagePipelineContext& DamageContext)
{
	// Initialize DoTContext to default values to make damage instant.
	constexpr FDamagePipelineEffectOverTimeContext EffectOverTimeContext;
	FGameplayEffectSpecHandle DamageSpecHandle = ConstructDamageEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::Instant, EffectOverTimeContext);

	if (DamageSpecHandle.IsValid())
	{
		return ApplyDamageToTarget_Internal(Target, Instigator, Damage, DamageContext, DamageSpecHandle);
	}

	return false;
}

bool UGASC_DamagePipelineSubsystem::ApplyHealToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Heal,
	const FDamagePipelineContext& HealContext)
{
	constexpr FDamagePipelineEffectOverTimeContext EffectOverTimeContext;
	FGameplayEffectSpecHandle HealingSpecHandle = ConstructHealingEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::Instant, EffectOverTimeContext);

	if (HealingSpecHandle.IsValid())
	{
		return ApplyHealToTarget_Internal(Target, Instigator, Heal, HealContext, HealingSpecHandle);
	}

	return false;
}

bool UGASC_DamagePipelineSubsystem::ApplyDamageToTarget_Internal(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	const FDamagePipelineContext& DamageContext,
	FGameplayEffectSpecHandle DamageSpecHandle)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ApplyDamageToTarget_Internal);

	AActor* TargetActor     = Target.Get();
	AActor* InstigatorActor = Instigator.Get();
	if (!TargetActor || !InstigatorActor)
	{
		return false;
	}

	FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get();
	if (!DamageSpec)
		return false;

	// Resolve PlayerState → Pawn → Character once
	if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(InstigatorActor))
	{
		InstigatorActor = PS->GetPawn();
	}
	if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(TargetActor))
	{
		TargetActor = PS->GetPawn();
	}

	AGASCourseCharacter* TargetChar     = Cast<AGASCourseCharacter>(TargetActor);
	AGASCourseCharacter* InstigatorChar = Cast<AGASCourseCharacter>(InstigatorActor);
	if (!TargetChar || !InstigatorChar)
	{
		return false;
	}

	UGASCourseAbilitySystemComponent* TargetASC     = TargetChar->GetAbilitySystemComponent();
	UGASCourseAbilitySystemComponent* InstigatorASC = InstigatorChar->GetAbilitySystemComponent();
	if (!TargetASC || !InstigatorASC)
	{
		return false;
	}

	// Add hit result if present
	if (DamageContext.HitResult.bBlockingHit)
	{
		DamageSpec->GetContext().AddHitResult(DamageContext.HitResult);
	}

	// Set damage magnitude
	DamageSpec->SetSetByCallerMagnitude(Data_IncomingDamage, Damage);

	// Tagging
	DamageSpec->DynamicGrantedTags.AppendTags(DamageContext.GrantedTags);
	DamageSpec->DynamicGrantedTags.AddTag(DamageContext.DamageType);
	
	// 1. Get original context
	FGameplayEffectContextHandle Original = DamageSpec->GetEffectContext();

	// 2. Duplicate into a writable one
	FGameplayEffectContextHandle NewContext = Original.Duplicate();

	// 3. Modify your custom data
	FGASCourseGameplayEffectContext* MutableContext =
		static_cast<FGASCourseGameplayEffectContext*>(NewContext.Get());
	if (DamageContext.HitResult.bBlockingHit)
	{
		//MutableContext->AddHitResult(DamageContext.HitResult);
		MutableContext->DamageLogEntry.HitResult = DamageContext.HitResult;
		DamageSpec->SetContext(NewContext);
	}

	// Apply
	TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec);

	return true;
}

bool UGASC_DamagePipelineSubsystem::ApplyHealToTarget_Internal(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Heal,
	const FDamagePipelineContext& HealContext,
	FGameplayEffectSpecHandle HealSpecHandle)
{
	SCOPED_NAMED_EVENT(ApplyHealingToTarget_Internal, FColor::Green);

	AActor* TargetActor     = Target.Get();
	AActor* InstigatorActor = Instigator.Get();
	if (!TargetActor || !InstigatorActor)
	{
		return false;
	}

	FGameplayEffectSpec* HealingSpec = HealSpecHandle.Data.Get();
	if (!HealingSpec)
		return false;

	AGASCourseCharacter* TargetChar     = Cast<AGASCourseCharacter>(Target.Get());
	AGASCourseCharacter* InstigatorChar = Cast<AGASCourseCharacter>(Instigator.Get());
	if (!TargetChar || !InstigatorChar)
	{
		return false;
	}

	UGASCourseAbilitySystemComponent* TargetASC     = TargetChar->GetAbilitySystemComponent();
	UGASCourseAbilitySystemComponent* InstigatorASC = InstigatorChar->GetAbilitySystemComponent();
	if (!TargetASC || !InstigatorASC)
	{
		return false;
	}

	HealingSpec->SetSetByCallerMagnitude(Data_IncomingHealing, Heal);

	if (!HealContext.GrantedTags.IsEmpty())
	{
		HealingSpec->DynamicGrantedTags.AppendTags(HealContext.GrantedTags);
	}

	if (HealContext.DamageType.IsValid())
	{
		HealingSpec->DynamicGrantedTags.AddTag(HealContext.DamageType);
	}
	
	// 1. Get original context
	FGameplayEffectContextHandle Original = HealingSpec->GetEffectContext();

	// 2. Duplicate into a writable one
	FGameplayEffectContextHandle NewContext = Original.Duplicate();

	// 3. Modify your custom data
	FGASCourseGameplayEffectContext* MutableContext =
		static_cast<FGASCourseGameplayEffectContext*>(NewContext.Get());
	if (HealContext.HitResult.bBlockingHit)
	{
		//MutableContext->AddHitResult(DamageContext.HitResult);
		MutableContext->DamageLogEntry.HitResult = HealContext.HitResult;
		HealingSpec->SetContext(NewContext);
	}
	
	TargetASC->ApplyGameplayEffectSpecToSelf(*HealingSpec);

	return true;
}

bool UGASC_DamagePipelineSubsystem::ApplyDamageToTargetDataHandle(
	const FGameplayAbilityTargetDataHandle& TargetHandle,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	FDamagePipelineContext DamageContext)
{
	TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetHandle);
	bool bDamageApplied = false;

	for (int32 TargetIdx = 0; TargetIdx < Targets.Num(); ++TargetIdx)
	{
		if (!DamageContext.HitResult.bBlockingHit)
		{
			DamageContext.HitResult =
				UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetHandle, TargetIdx);
		}

		bDamageApplied = ApplyDamageToTarget(Targets[TargetIdx], Instigator, Damage, DamageContext);
	}

	return bDamageApplied;
}

bool UGASC_DamagePipelineSubsystem::ApplyHealToTargetDataHandle(
	const FGameplayAbilityTargetDataHandle& TargetHandle,
	TWeakObjectPtr<AActor> Instigator,
	float Heal,
	FDamagePipelineContext HealContext)
{
	TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetHandle);
	bool bHealingApplied = false;

	for (int32 TargetIdx = 0; TargetIdx < Targets.Num(); ++TargetIdx)
	{
		if (!HealContext.HitResult.bBlockingHit)
		{
			HealContext.HitResult =
				UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetHandle, TargetIdx);
		}

		bHealingApplied = ApplyHealToTarget(Targets[TargetIdx], Instigator, Heal, HealContext);
	}

	return bHealingApplied;
}

bool UGASC_DamagePipelineSubsystem::ApplyDamageOverTimeToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	const FDamagePipelineContext& DamageContext,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	FGameplayEffectSpecHandle DamageSpecHandle = ConstructDamageEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::HasDuration, EffectOverTimeContext);

	if (DamageSpecHandle.IsValid())
	{
		return ApplyDamageToTarget_Internal(Target, Instigator, Damage, DamageContext, DamageSpecHandle);
	}

	return false;
}

bool UGASC_DamagePipelineSubsystem::ApplyHealOverTimeToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Heal,
	const FDamagePipelineContext& HealContext,
	const FDamagePipelineEffectOverTimeContext& EffectOverTimeContext)
{
	FGameplayEffectSpecHandle HealOverTimeSpecHandle = ConstructHealingEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::HasDuration, EffectOverTimeContext);

	if (HealOverTimeSpecHandle.IsValid())
	{
		return ApplyHealToTarget_Internal(Target, Instigator, Heal, HealContext, HealOverTimeSpecHandle);
	}

	return false;
}

bool UGASC_DamagePipelineSubsystem::ApplyPhysicalDamageToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	const FHitResult& HitResult,
	FDamagePipelineContext& DamageContext)
{
	DamageContext.DamageType = DamageType_Physical;
	DamageContext.HitResult  = HitResult;

	constexpr FDamagePipelineEffectOverTimeContext EffectOverTimeContext;

	FGameplayEffectSpecHandle DamageSpecHandle = ConstructDamageEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::Instant, EffectOverTimeContext);

	if (DamageSpecHandle.IsValid())
	{
		return ApplyDamageToTarget_Internal(Target, Instigator, Damage, DamageContext, DamageSpecHandle);
	}

	return false;
}

bool UGASC_DamagePipelineSubsystem::ApplyFireDamageToTarget(
	TWeakObjectPtr<AActor> Target,
	TWeakObjectPtr<AActor> Instigator,
	float Damage,
	const FHitResult& HitResult,
	FDamagePipelineContext& DamageContext,
	bool bApplyBurnStack /*= true*/)
{
	DamageContext.DamageType = DamageType_Elemental_Fire;

	if (bApplyBurnStack)
	{
		FGameplayTagContainer GrantedTags;
		GrantedTags.AddTag(FGameplayTag::RequestGameplayTag(
			FName("Effect.Gameplay.Status.Burn.Stack")));
		DamageContext.GrantedTags = GrantedTags;
	}

	DamageContext.HitResult = HitResult;

	constexpr FDamagePipelineEffectOverTimeContext EffectOverTimeContext;

	FGameplayEffectSpecHandle DamageSpecHandle = ConstructDamageEffectSpecHandle(
		Instigator, EGameplayEffectDurationType::Instant, EffectOverTimeContext);

	if (DamageSpecHandle.IsValid())
	{
		return ApplyDamageToTarget_Internal(Target, Instigator, Damage, DamageContext, DamageSpecHandle);
	}

	return false;
}
