// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "GASC_AbilityTickTask.generated.h"
 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickTaskDelegate, float, DeltaTime);
 
/**
 * Task for abilities that supply tick and its' delta time.
 */

UCLASS()
class GASCOURSE_API UGASC_AbilityTickTask : public UAbilityTask
{
	GENERATED_BODY()
 
	UPROPERTY(BlueprintAssignable)
	FOnTickTaskDelegate OnTick;
 
public:
 
	UGASC_AbilityTickTask(const FObjectInitializer& ObjectInitializer);
 
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGASC_AbilityTickTask* AbilityTaskOnTick(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName);
	
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	
};