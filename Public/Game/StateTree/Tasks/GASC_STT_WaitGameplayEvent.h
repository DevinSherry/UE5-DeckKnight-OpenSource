#pragma once

#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StateTreeDelegate.h"
#include "Delegates/Delegate.h"

#include "GASC_STT_WaitGameplayEvent.generated.h"

struct FGameplayEventData;
class UAbilitySystemComponent;

/**
 * Instance data (per execution)
 */
USTRUCT()
struct FGASC_STT_WaitGameplayEventInstanceData
{
    GENERATED_BODY()
	
	/* -------- Parameters -------- */

	UPROPERTY(EditAnywhere, Category = "Parameters")
	FGameplayTag GameplayEventTag;
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	bool bMatchTagExact = true;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	bool bOnlyTriggerOnce = true;

	UPROPERTY(EditAnywhere, Category = "Output")
	FStateTreeDelegateDispatcher GameplayEventDelegate;

    UPROPERTY(Transient)
    bool bEventReceived = false;

    UPROPERTY(Transient)
    bool bIsActive = false;
	
    FDelegateHandle DelegateHandle;

};

/**
 * Native StateTree task
 */
USTRUCT(meta = (DisplayName = "Wait Gameplay Event (GAS)"))
struct GASCOURSE_API FGASC_STT_WaitGameplayEvent
    : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
	
	FGASC_STT_WaitGameplayEvent();

    using FInstanceDataType = FGASC_STT_WaitGameplayEventInstanceData;

    /* -------- StateTree Overrides -------- */

    virtual const UStruct* GetInstanceDataType() const override
    {
        return FInstanceDataType::StaticStruct();
    }

    virtual EStateTreeRunStatus EnterState(
        FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

    virtual EStateTreeRunStatus Tick(
        FStateTreeExecutionContext& Context,
        float DeltaTime) const override;

    virtual void ExitState(
        FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
    virtual FText GetDescription(
        const FGuid& ID,
        FStateTreeDataView InstanceDataView,
        const IStateTreeBindingLookup& BindingLookup,
        EStateTreeNodeFormatting Formatting) const override;
#endif

private:
    /* -------- GAS callbacks -------- */

    void OnGameplayEventExact(
        const FGameplayEventData* Payload,
        FGASC_STT_WaitGameplayEventInstanceData& InstanceData) const;

    void OnGameplayEventContainer(
        FGameplayTag MatchingTag,
        const FGameplayEventData* Payload,
        FGASC_STT_WaitGameplayEventInstanceData& InstanceData) const;
};