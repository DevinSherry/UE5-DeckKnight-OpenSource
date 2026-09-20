#include "Game/StateTree/Tasks/GASC_StateTreeTask_InputListener.h"

#include "StateTreeExecutionContext.h"
#include "EnhancedInputComponent.h"
#include "Components/StateTreeComponent.h"

#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"

// Namespaced rather than left as file statics: GASC_StateTreeTaskBlockStateTreeTransition.cpp
// declares four of these with the same signatures, and a unity build compiles both files into one
// translation unit, where two definitions of the same static function is a redefinition error.
namespace GASCourse::StateTreeInputListener
{
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
        if (AGASCoursePlayerCharacter* Char = StateTreeInputListener::GetPlayerChar(PC))
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
}

// An alias rather than a using-directive. A using-directive would hoist GetPC and friends into the
// global scope of the *whole* unity translation unit, where they would be ambiguous against the
// identically-named helpers the other file hoists - which is the error this replaces. The alias only
// introduces the short namespace name, and that name differs per file.
namespace StateTreeInputListener = GASCourse::StateTreeInputListener;

EStateTreeRunStatus FStateTreeTask_GASCInputListener::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    if (!Context.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    FInstanceDataType& Data = Context.GetInstanceData(*this);

    APlayerController* PC = StateTreeInputListener::GetPC(Context);
    if (!PC)
    {
        return EStateTreeRunStatus::Failed;
    }

    UEnhancedInputComponent* InputComp = StateTreeInputListener::GetEnhancedInput(PC);
    UStateTreeComponent* ST = StateTreeInputListener::GetStateTree(PC);
    UGASC_InputBufferComponent* InputBuffer = StateTreeInputListener::GetInputBuffer(PC);

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

                    	FGameplayTag InputCategoryTag = WeakBuffer->FindCategoryTagForInputAction(Action);
                    	bool bWasBufferedAction = WeakBuffer->IsInputBufferOpenForCategory(InputCategoryTag);
                        WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag, ETriggerEvent::Triggered, Value, bWasBufferedAction);
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

                    	FGameplayTag InputCategoryTag = WeakBuffer->FindCategoryTagForInputAction(Action);
						bool bWasBufferedAction = WeakBuffer->IsInputBufferOpenForCategory(InputCategoryTag);
						WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag, ETriggerEvent::Completed, Value, bWasBufferedAction);
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

                    	FGameplayTag InputCategoryTag = WeakBuffer->FindCategoryTagForInputAction(Action);
						bool bWasBufferedAction = WeakBuffer->IsInputBufferOpenForCategory(InputCategoryTag);
						WeakBuffer->QueueStateTreeEventOncePerActionPerFrame(Action, Tag, ETriggerEvent::Canceled, Value, bWasBufferedAction);
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

        if (APlayerController* PC = StateTreeInputListener::GetPC(Context))
        {
            if (UEnhancedInputComponent* InputComp = StateTreeInputListener::GetEnhancedInput(PC))
            {
                // Correct Enhanced Input removal: RemoveBindingByHandle(handle) [1](https://issuetracker.google.com/issues/149630915)[2](https://learn.microsoft.com/en-us/answers/questions/1648395/how-to-fix-debug-assertion-failed-error)
                for (uint32 Handle : Data.InputBindingHandles)
                {
                    InputComp->RemoveBindingByHandle(Handle);
                }
            }

            if (UGASC_InputBufferComponent* Buffer = StateTreeInputListener::GetInputBuffer(PC))
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

    APlayerController* PC = StateTreeInputListener::GetPC(Context);
    if (!PC)
    {
        return EStateTreeRunStatus::Failed;
    }

    UStateTreeComponent* ST = StateTreeInputListener::GetStateTree(PC);
    UGASC_InputBufferComponent* Buffer = StateTreeInputListener::GetInputBuffer(PC);

    if (!ST || !Buffer)
    {
        return EStateTreeRunStatus::Failed;
    }

	//TODO check for specific tag category buffered
    FGameplayTag Tag;
	FGASC_STInputEventPayload Payload;
	if (Buffer->HasPendingInputBufferEvent())
	{
		if (Buffer->ConsumeQueuedStateTreeEvent(Tag, Payload))
		{
			Data.LastDispatchFrame = GFrameCounter;
			ST->SendStateTreeEvent(Tag, FConstStructView::Make(Payload));
		}
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
