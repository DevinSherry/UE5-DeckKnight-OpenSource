#include "Game/StateTree/Tasks/GASC_StateTreeTask_InputListener.h"

#include "StateTreeExecutionContext.h"
#include "EnhancedInputComponent.h"
#include "Components/StateTreeComponent.h"

#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"

static APlayerController* GetPC(FStateTreeExecutionContext& Context)
{
    return Cast<APlayerController>(Context.GetOwner());
}

static AGASCoursePlayerCharacter* GetPlayerChar(APlayerController* PC)
{
    return PC ? Cast<AGASCoursePlayerCharacter>(PC->GetPawn()) : nullptr;
}

static UGASC_InputBufferComponent* GetInputBuffer(APlayerController* PC)
{
    if (AGASCoursePlayerCharacter* Char = GetPlayerChar(PC))
    {
        return Char->GetInputBufferComponent();
    }
    return nullptr;
}

static UEnhancedInputComponent* GetEnhancedInput(APlayerController* PC)
{
    return PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
}

static UStateTreeComponent* GetStateTree(APlayerController* PC)
{
    return PC ? PC->GetComponentByClass<UStateTreeComponent>() : nullptr;
}

EStateTreeRunStatus FStateTreeTask_GASCInputListener::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    if (!Context.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    FInstanceDataType& Data = Context.GetInstanceData(*this);

    APlayerController* PC = GetPC(Context);
    if (!PC)
    {
        return EStateTreeRunStatus::Failed;
    }

    UEnhancedInputComponent* InputComp = GetEnhancedInput(PC);
    UStateTreeComponent* ST = GetStateTree(PC);
    UGASC_InputBufferComponent* InputBuffer = GetInputBuffer(PC);

    if (!InputComp || !ST || !InputBuffer)
    {
        return EStateTreeRunStatus::Failed;
    }

    Data.InputBindingHandles.Reset();
    Data.LastDispatchFrame = MAX_uint64;

    // Weak pointers for safety inside callbacks.
    const TWeakObjectPtr<UGASC_InputBufferComponent> WeakBuffer(InputBuffer);

    for (const FEnhancedInputListenerData& InputData : Data.InputListeners)
    {
        if (!InputData.InputAction)
        {
            continue;
        }

        const UInputAction* Action = InputData.InputAction;

        // NOTE: If you only want one-shot presses, change Triggered to Started.
        // We keep Triggered here, but the buffer component will gate "once per action per frame".

        if (InputData.TriggeredEventGameplayTag.IsValid())
        {
            const FGameplayTag Tag = InputData.TriggeredEventGameplayTag;

            FEnhancedInputActionEventBinding& Binding =
                InputComp->BindActionValueLambda(Action, ETriggerEvent::Triggered,
                    [WeakBuffer, Action, Tag](const FInputActionValue& Value)
                    {
                        if (!WeakBuffer.IsValid())
                            return;

                        WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag);
                    }); // BindActionValueLambda [3](https://forums.unrealengine.com/t/statetree-bug-binding-to-a-uproperty-held-by-a-output-uobject/1346674)

            Data.InputBindingHandles.Add(Binding.GetHandle()); // FEnhancedInputActionEventBinding has GetHandle [2](https://learn.microsoft.com/en-us/answers/questions/1648395/how-to-fix-debug-assertion-failed-error)
        }

        if (InputData.CompletedGameplayTag.IsValid())
        {
            const FGameplayTag Tag = InputData.CompletedGameplayTag;

            FEnhancedInputActionEventBinding& Binding =
                InputComp->BindActionValueLambda(Action, ETriggerEvent::Completed,
                    [WeakBuffer, Action, Tag](const FInputActionValue& Value)
                    {
                        if (!WeakBuffer.IsValid())
                            return;

                        WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag);
                    });

            Data.InputBindingHandles.Add(Binding.GetHandle());
        }

        if (InputData.CanceledGameplayTag.IsValid())
        {
            const FGameplayTag Tag = InputData.CanceledGameplayTag;

            FEnhancedInputActionEventBinding& Binding =
                InputComp->BindActionValueLambda(Action, ETriggerEvent::Canceled,
                    [WeakBuffer, Action, Tag](const FInputActionValue& Value)
                    {
                        if (!WeakBuffer.IsValid())
                            return;

                        WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag);
                    });

            Data.InputBindingHandles.Add(Binding.GetHandle());
        }
    }

    return EStateTreeRunStatus::Running;
}

void FStateTreeTask_GASCInputListener::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    if (Context.IsValid())
    {
        FInstanceDataType& Data = Context.GetInstanceData(*this);

        if (APlayerController* PC = GetPC(Context))
        {
            if (UEnhancedInputComponent* InputComp = GetEnhancedInput(PC))
            {
                // Correct Enhanced Input removal: RemoveBindingByHandle(handle) [1](https://issuetracker.google.com/issues/149630915)[2](https://learn.microsoft.com/en-us/answers/questions/1648395/how-to-fix-debug-assertion-failed-error)
                for (uint32 Handle : Data.InputBindingHandles)
                {
                    InputComp->RemoveBindingByHandle(Handle);
                }
            }

            if (UGASC_InputBufferComponent* Buffer = GetInputBuffer(PC))
            {
                Buffer->ResetStateTreeEventQueue();
            }
        }

        Data.InputBindingHandles.Reset();
        Data.LastDispatchFrame = MAX_uint64;
    }

    FStateTreeTaskCommonBase::ExitState(Context, Transition);
}

EStateTreeRunStatus FStateTreeTask_GASCInputListener::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    if (!Context.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    FInstanceDataType& Data = Context.GetInstanceData(*this);

    // Hard gate: at most one dispatch per frame from this task (optional but nice).
    if (Data.LastDispatchFrame == GFrameCounter)
    {
        return EStateTreeRunStatus::Running;
    }

    APlayerController* PC = GetPC(Context);
    if (!PC)
    {
        return EStateTreeRunStatus::Failed;
    }

    UStateTreeComponent* ST = GetStateTree(PC);
    UGASC_InputBufferComponent* Buffer = GetInputBuffer(PC);

    if (!ST || !Buffer)
    {
        return EStateTreeRunStatus::Failed;
    }

    FGameplayTag Tag;
    if (Buffer->ConsumeQueuedStateTreeEvent(Tag))
    {
        Data.LastDispatchFrame = GFrameCounter;
        ST->SendStateTreeEvent(Tag); // SendStateTreeEvent [4](https://unrealist.org/dev-log-03-statetree-isnt-just-for-ai/)
    }

    return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeTask_GASCInputListener::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
    const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
    return FStateTreeTaskCommonBase::GetDescription(ID, InstanceDataView, BindingLookup, Formatting);
}
#endif