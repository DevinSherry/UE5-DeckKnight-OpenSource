// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineSubsystem.h"
#include "GASC_OnDamageEventTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageEventDelegate, const FResourceModificationContext&, DamageEvent);

/**
 * @class UUGASC_OnDamageEventTask
 * @brief A gameplay task designed to handle on-damage events within the Unreal Engine Gameplay Ability System framework.
 *
 * This class allows the user to bind behavior to damage events, enabling custom handling of damage responses
 * during gameplay. The task responds to damages received by the associated actor or component and can be used
 * for mechanics such as triggering specific abilities, visual effects, or sound effects upon taking damage.
 *
 * Features include:
 * - Subscribing to damage event callbacks to execute conditional or custom logic.
 * - Support for filter or condition-based trigger mechanisms to ensure granular control over damage handling.
 *
 */
UCLASS()
class GASCOURSE_API UGASC_OnDamageEventTask : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnDamageEventDelegate OnDamageEvent;
	
	UFUNCTION()
	void OnDamageEventReceived(const FResourceModificationContext& ResourceModificationContextContext);
	
	UFUNCTION()
	void OnDamageEventApplied(const FResourceModificationContext& ResourceModificationContextContext);
	
	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;
	
	UFUNCTION(BlueprintCallable, Category="Abiltiy|Tasks", meta = (HidePin="OwningAbility", DefaultToSelf="OwningAbility", 
		BlueprintInternalUseOnly="TRUE"))
	static UGASC_OnDamageEventTask* WaitOnDamageEvent(
		UGameplayAbility* OwningAbility,
		AActor* OptionalExternalTarget = nullptr,
		EOnDamageEventType DamageEventToListenFor = OnDamageApplied);
	
	/** External target override */
	void SetExternalTarget(AActor* Actor);

	/** Which actor should the event be filtered for? */
	AActor* GetTarget() const;
	
private:

	/** Applied or Received */
	EOnDamageEventType DamageEventToListenFor;

	/** Optional externally-specified target */
	UPROPERTY()
	TWeakObjectPtr<AActor> OptionalExternalTarget;

	/** Default target comes from the ability's avatar */
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/** Did we override the target? */
	bool bUseExternalTarget = false;
};
