// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "GASC_DamagePipelineTypes.h"
#include "GASC_DamagePipelineSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_DamagePipelineSubsystem, Log, All);

/**
 * UGASC_DamagePipelineSubsystem is a subsystem designed for handling
 * hit and damage pipelines within the game framework. It simplifies
 * communication and processing of hit events, managing related
 * data transfer across characters or systems.
 *
 * This class is particularly useful in gameplay mechanics
 * involving damage calculations, applying hit reactions, or
 * communicating hit-related gameplay events.
 */
UCLASS()
class GASCOURSE_API UGASC_DamagePipelineSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Blueprint-accessible global delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Damage Pipeline")
	FOnHitApplied OnHitAppliedDelegateCallback;

	// --- Blueprint-accessible global delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Damage Pipeline")
	FOnHitReceived OnHitReceivedDelegateCallback;

public:

	UFUNCTION()
	bool OnHitEvent(const FHitContext& HitContext);

	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	bool SendHitRequest(const FHitContext& HitContext);

	UFUNCTION(BlueprintCallable, Category = "Damage Pipeline")
	void CallbackTester(FOnHitApplied_Event callback);

	UFUNCTION()
	void Internal_ForwardOnHitApplied(const FHitContext& HitContext, TWeakObjectPtr<AActor> Listener);

	FOnHitApplied_Event ForwardedCallback;
	
};