// GASC_OnHitEventTask.h - Refactored for unified Damage Pipeline

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "GASC_OnHitEventTask.generated.h"

// Public delegate exposed to abilities / blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGASCWaitForHitEventDelegate, const FHitContext&, HitContext);

UCLASS()
class GASCOURSE_API UGASC_OnHitEventTask : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

public:

	/** Called whenever the hit event occurs */
	UPROPERTY(BlueprintAssignable)
	FGASCWaitForHitEventDelegate OnHitDelegate;

	/** Internal handlers */
	UFUNCTION()
	void HandleHitApplied(const FHitContext& HitContext);

	UFUNCTION()
	void HandleHitReceived(const FHitContext& HitContext);

	/** AbilityTask overrides */
	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;

	/** Factory method */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks",
		meta = (HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="TRUE"))
	static UGASC_OnHitEventTask* WaitOnHitEvent(
		UGameplayAbility* OwningAbility,
		AActor* OptionalExternalTarget = nullptr,
		EHitEventType HitEvent = EHitEventType::OnHitApplied);

	/** External target override */
	void SetExternalTarget(AActor* Actor);

	/** Which actor should the event be filtered for? */
	AActor* GetTarget() const;

private:

	/** Applied or Received */
	EHitEventType HitEventToListenFor;

	/** Optional externally-specified target */
	UPROPERTY()
	TWeakObjectPtr<AActor> OptionalExternalTarget;

	/** Default target comes from the ability's avatar */
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/** Did we override the target? */
	bool bUseExternalTarget = false;
};
