// GASC_DamagePipelineDelegates.h
// Unified delegate file for Hit / Damage / Healing

#pragma once

#include "GASC_ResourcePipelineDelegates.generated.h"

struct FGameplayAttribute;
// Forward declare context structs
struct FHitContext;
struct FResourceModificationContext;

/*
====================================================================
   GLOBAL BLUEPRINT MULTICAST EVENTS (UI, HUD, FX)
====================================================================
*/

// Hit
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitApplied_BP,    const FHitContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitReceived_BP,   const FHitContext&, Context);

// Damage
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageApplied_BP,  const FResourceModificationContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageReceived_BP, const FResourceModificationContext&, Context);

// Healing
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingApplied_BP,  const FResourceModificationContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingReceived_BP, const FResourceModificationContext&, Context);

// One uniform delegate signature for every attribute
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceModified_BP, const FResourceModificationContext&, Context);


/*
====================================================================
   PER-OBJECT SINGLE-CAST DELEGATES (Blueprint registration API)
====================================================================
*/

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHitApplied_Event,    const FHitContext&, Context);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDamageApplied_Event, const FResourceModificationContext&, Context);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHealingApplied_Event,const FResourceModificationContext&, Context);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnResourceModified_Event, const FResourceModificationContext&, Context);

/*
====================================================================
	 NATIVE FAST-PATH DELEGATES (C++ ONLY, HIGH-PERFORMANCE)
====================================================================
*/

// Hit
DECLARE_DELEGATE_OneParam(FOnHitAppliedNative,    const FHitContext&);
DECLARE_DELEGATE_OneParam(FOnHitReceivedNative,   const FHitContext&);

// Damage
DECLARE_DELEGATE_OneParam(FOnDamageAppliedNative,  const FResourceModificationContext&);
DECLARE_DELEGATE_OneParam(FOnDamageReceivedNative, const FResourceModificationContext&);

// Healing
DECLARE_DELEGATE_OneParam(FOnHealingAppliedNative,  const FResourceModificationContext&);
DECLARE_DELEGATE_OneParam(FOnHealingReceivedNative, const FResourceModificationContext&);

DECLARE_DELEGATE_OneParam(FOnResourceModifiedNative, const FResourceModificationContext&);

/*
====================================================================
   NATIVE LISTENER STRUCTS
====================================================================
*/

USTRUCT()
struct FNativeHitListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> Listener;

	FOnHitAppliedNative  OnApplied;
	FOnHitReceivedNative OnReceived;
};

USTRUCT()
struct FNativeDamageListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> Listener;

	FOnDamageAppliedNative  OnApplied;
	FOnDamageReceivedNative OnReceived;
};

USTRUCT()
struct FNativeHealingListener
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> Listener;

	FOnHealingAppliedNative  OnApplied;
	FOnHealingReceivedNative OnReceived;
};

USTRUCT()
struct FResourceListenerEntry
{
	GENERATED_BODY()
	UPROPERTY() TWeakObjectPtr<UObject> Listener;
	FOnResourceModifiedNative NativeCallback;   // one of these is bound
	UPROPERTY() FOnResourceModified_Event DynamicCallback;
};

struct FAttributeListenerBucket
{
	TArray<FResourceListenerEntry> Applied;
	TArray<FResourceListenerEntry> Received;
};

// The whole subsystem state collapses to:
TMap<FGameplayAttribute, FAttributeListenerBucket> ResourceModificationListeners;
