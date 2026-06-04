#include "Game/StateTree/Tasks/GASC_STT_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "StateTreeExecutionContext.h"

static UAbilitySystemComponent* ResolveASC(AActor* Actor)
{
    return Actor
        ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor)
        : nullptr;
}

FGASC_STT_WaitGameplayEvent::FGASC_STT_WaitGameplayEvent()
{
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FGASC_STT_WaitGameplayEvent::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    auto& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

    InstanceData.bEventReceived = false;
    InstanceData.bIsActive = true;
    InstanceData.DelegateHandle.Reset();
	
	if (!InstanceData.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}
	
    UAbilitySystemComponent* ASC = ResolveASC(InstanceData.TargetActor);

    if (!ASC || !InstanceData.GameplayEventTag.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    if (InstanceData.bMatchTagExact)
    {
        InstanceData.DelegateHandle =
            ASC->GenericGameplayEventCallbacks
                .FindOrAdd(InstanceData.GameplayEventTag)
                .AddLambda([this, &InstanceData](const FGameplayEventData* Payload)
                {
                    OnGameplayEventExact(Payload, InstanceData);
                });
    }
    else
    {
        InstanceData.DelegateHandle =
            ASC->AddGameplayEventTagContainerDelegate(
                FGameplayTagContainer(InstanceData.GameplayEventTag),
                FGameplayEventTagMulticastDelegate::FDelegate::CreateLambda(
                    [this, &InstanceData](FGameplayTag Tag, const FGameplayEventData* Payload)
                    {
                        OnGameplayEventContainer(Tag, Payload, InstanceData);
                    }));
    }

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FGASC_STT_WaitGameplayEvent::Tick(
    FStateTreeExecutionContext& Context,
    float DeltaTime) const
{
    auto& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

    if (!InstanceData.bIsActive)
    {
        return EStateTreeRunStatus::Failed;
    }

    if (InstanceData.bEventReceived)
    {
        if (InstanceData.GameplayEventDelegate.IsValid())
        {
            Context.BroadcastDelegate(InstanceData.GameplayEventDelegate);
        }

        InstanceData.bEventReceived = false;

        if (InstanceData.bOnlyTriggerOnce)
        {
            return EStateTreeRunStatus::Succeeded;
        }
    }

    return EStateTreeRunStatus::Running;
}

void FGASC_STT_WaitGameplayEvent::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    auto& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	

    if (UAbilitySystemComponent* ASC = ResolveASC(InstanceData.TargetActor))
    {
        if (InstanceData.DelegateHandle.IsValid())
        {
            if (InstanceData.bMatchTagExact)
            {
                ASC->GenericGameplayEventCallbacks
                    .FindOrAdd(InstanceData.GameplayEventTag)
                    .Remove(InstanceData.DelegateHandle);
            }
            else
            {
            	FGameplayTagContainer TagContainer(InstanceData.GameplayEventTag);
                ASC->RemoveGameplayEventTagContainerDelegate(TagContainer, InstanceData.DelegateHandle);
            }
        }
    }

    InstanceData.bIsActive = false;
    InstanceData.bEventReceived = false;
    InstanceData.DelegateHandle.Reset();
}

void FGASC_STT_WaitGameplayEvent::OnGameplayEventExact(
    const FGameplayEventData* Payload,
    FGASC_STT_WaitGameplayEventInstanceData& InstanceData) const
{
    if (InstanceData.bIsActive)
    {
        InstanceData.bEventReceived = true;
    }
}

void FGASC_STT_WaitGameplayEvent::OnGameplayEventContainer(
    FGameplayTag MatchingTag,
    const FGameplayEventData* Payload,
    FGASC_STT_WaitGameplayEventInstanceData& InstanceData) const
{
    if (InstanceData.bIsActive)
    {
        InstanceData.bEventReceived = true;
    }
}

#if WITH_EDITOR
FText FGASC_STT_WaitGameplayEvent::GetDescription(
    const FGuid& ID,
    FStateTreeDataView InstanceDataView,
    const IStateTreeBindingLookup& BindingLookup,
    EStateTreeNodeFormatting Formatting) const
{
    const FGASC_STT_WaitGameplayEventInstanceData& InstanceData = InstanceDataView.Get<FInstanceDataType>();
    return FText::Format(
        NSLOCTEXT("GASC", "WaitGameplayEventDesc",
            "Wait for Gameplay Event [{0}]"),
        FText::FromName(InstanceData.GameplayEventTag.GetTagName()));
}
#endif