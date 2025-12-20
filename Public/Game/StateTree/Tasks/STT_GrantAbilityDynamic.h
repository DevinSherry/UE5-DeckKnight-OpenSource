#pragma once

#include "StateTreeTaskBase.h"
#include "Abilities/GameplayAbility.h"
#include "STT_GrantAbilityDynamic.generated.h"

/**
 * StateTree Task: Grant a Gameplay Ability and expose its editable properties
 * dynamically as bindable pins.
 */

/** Instance Data holds per-node state and bindable properties */
USTRUCT()
struct FInstanceDataAbilityData
{
	GENERATED_BODY()

	/** Auto-generated property bag of ability parameters. */
	UPROPERTY(EditAnywhere, Category="Ability")
	FInstancedPropertyBag AbilityParams;

	/** Delegate binding handle for OnAbilityEnded */
	FDelegateHandle AbilityEndedDelegateHandle;
	
	bool bAbilityCancelled = false;
	bool bAbilityFailed = false;
	
	FGameplayAbilitySpecHandle GrantedHandle;
	bool bGrantedByTask   = false;
	bool bActivated       = false;
	bool bAbilityEnded    = false;

};

USTRUCT()
struct FGrantAbilityDynamicTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category="Ability")
	bool bForceRebuild = false;
#endif

	/** The Gameplay Ability class to grant. */
	UPROPERTY(EditAnywhere, Category="Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	using FInstanceDataType = FInstanceDataAbilityData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataAbilityData::StaticStruct(); }

	// === Task lifecycle ===
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	void UpdateAbilityParamsSchema(FInstancedPropertyBag& Bag, TSubclassOf<UGameplayAbility> AbilityClass);

#if WITH_EDITOR
	/** Regenerate property bag schema when AbilityClass changes. */
	virtual void PostEditNodeChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent, FStateTreeDataView InstanceDataView) override;
#endif
};