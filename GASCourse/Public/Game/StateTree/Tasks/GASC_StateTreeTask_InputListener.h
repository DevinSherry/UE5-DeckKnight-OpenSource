#pragma once

#include "InputAction.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "GASC_StateTreeTask_InputListener.generated.h"

class UStateTreeComponent;

USTRUCT(BlueprintType)
struct FEnhancedInputListenerData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    TObjectPtr<const UInputAction> InputAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    FGameplayTag TriggeredEventGameplayTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    FGameplayTag CompletedGameplayTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    FGameplayTag CanceledGameplayTag;
};

USTRUCT()
struct FGASC_StateTreeTask_InputListener_InstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="InputListener")
    TArray<FEnhancedInputListenerData> InputListeners;

    /** Handles returned by Enhanced Input bindings (safe to store/unbind). */
    UPROPERTY(Transient)
    TArray<uint32> InputBindingHandles;

    /** Optional: global per-frame dispatch gate for this task (not per-action). */
    UPROPERTY(Transient)
    uint64 LastDispatchFrame = MAX_uint64;
};

USTRUCT(meta=(DisplayName="GASC Input Listener (Safe Once-Per-Action-Per-Frame)"))
struct FStateTreeTask_GASCInputListener : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    using FInstanceDataType = FGASC_StateTreeTask_InputListener_InstanceData;

    virtual const UStruct* GetInstanceDataType() const override
    {
        return FInstanceDataType::StaticStruct();
    }

    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

    virtual void ExitState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;

    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
        const float DeltaTime) const override;

#if WITH_EDITOR
    virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
        const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const override;
#endif
};