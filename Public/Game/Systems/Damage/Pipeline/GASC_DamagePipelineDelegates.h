// GASC_DamagePipelineDelegates.h
// Unified delegate file for Hit / Damage / Healing

#pragma once

#include "GASC_DamagePipelineDelegates.generated.h"

// Forward declare context structs
struct FHitContext;
struct FDamageModificationContext;

/*
====================================================================
   GLOBAL BLUEPRINT MULTICAST EVENTS (UI, HUD, FX)
====================================================================
*/

// Hit
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitApplied_BP,    const FHitContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitReceived_BP,   const FHitContext&, Context);

// Damage
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageApplied_BP,  const FDamageModificationContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageReceived_BP, const FDamageModificationContext&, Context);

// Healing
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingApplied_BP,  const FDamageModificationContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingReceived_BP, const FDamageModificationContext&, Context);

/*
====================================================================
   PER-OBJECT SINGLE-CAST DELEGATES (Blueprint registration API)
====================================================================
*/

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHitApplied_Event,    const FHitContext&, Context);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDamageApplied_Event, const FDamageModificationContext&, Context);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHealingApplied_Event,const FDamageModificationContext&, Context);

/*
====================================================================
	 NATIVE FAST-PATH DELEGATES (C++ ONLY, HIGH-PERFORMANCE)
====================================================================
*/

// Hit
DECLARE_DELEGATE_OneParam(FOnHitAppliedNative,    const FHitContext&);
DECLARE_DELEGATE_OneParam(FOnHitReceivedNative,   const FHitContext&);

// Damage
DECLARE_DELEGATE_OneParam(FOnDamageAppliedNative,  const FDamageModificationContext&);
DECLARE_DELEGATE_OneParam(FOnDamageReceivedNative, const FDamageModificationContext&);

// Healing
DECLARE_DELEGATE_OneParam(FOnHealingAppliedNative,  const FDamageModificationContext&);
DECLARE_DELEGATE_OneParam(FOnHealingReceivedNative, const FDamageModificationContext&);

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
