// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/Input/GASCourse_WaitButtonDown.h"
#include "EnhancedPlayerInput.h"

UGASCourse_WaitInputStateChange::UGASCourse_WaitInputStateChange(const FObjectInitializer& ObjectInitializer)
{
	bTickingTask = true;
}

void UGASCourse_WaitInputStateChange::Activate()
{
	Super::Activate();
	
	AActor* Actor = TargetActor.Get();
	if (!IsValid(Actor))
	{
		EndTask();
		return;
	}
	
	if (!InputActionToListen)
	{
		EndTask();
		return;
	}
	
	const APlayerController* PC = Ability ? Ability->GetCurrentActorInfo()->PlayerController.Get() : nullptr;
	EnhancedPlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
	if (!EnhancedPlayerInput.Get())
	{
		EndTask();
		return;
	}
}

void UGASCourse_WaitInputStateChange::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

void UGASCourse_WaitInputStateChange::OnGameplayTaskActivated(UGameplayTask& Task)
{
	Super::OnGameplayTaskActivated(Task);
	CheckInputState(true);
}

UGASCourse_WaitInputStateChange* UGASCourse_WaitInputStateChange::WaitButtonStateChanged(UGameplayAbility* OwningAbility, UInputAction* InputAction,
                                                                                         EGASCourseInputStateChange InputStateChangeToListen, bool bTriggerOnce)
{
	UGASCourse_WaitInputStateChange* Task = NewAbilityTask<UGASCourse_WaitInputStateChange>(OwningAbility);
	Task->InputActionToListen = InputAction;
	Task->InputStateChangeToListenFor = InputStateChangeToListen;
	Task->bTriggerOnce = bTriggerOnce;
	
	// Default target is the ability avatar
	Task->TargetActor = OwningAbility->GetAvatarActorFromActorInfo();
	
	return Task;
}

void UGASCourse_WaitInputStateChange::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (bTaskTriggered)
	{
		EndTask();
		return;
	}
	
	if (!EnhancedPlayerInput.Get())
	{
		return;
	}
	
	if (IsActive())
	{
		CheckInputState(!bInitialCheckDone);
	}
}

void UGASCourse_WaitInputStateChange::CheckInputState(bool bInitialCheck)
{

	const FInputActionInstance* ActionInstance = EnhancedPlayerInput->FindActionInstanceData(InputActionToListen);
	if (!ActionInstance)
	{
		return;   // not mapped / not yet evaluated — do NOT infer "up"
	}

	const bool bIsDown = ActionInstance->GetValue().Get<bool>();
	
	if (!bInitialCheck)
	{
		// Seed the baseline on the first sample so we only ever report a transition.
		if (!bHasPrevState)
		{
			bHasPrevState = true;
			bPrevDown = bIsDown;
			return;
		}

		if (bIsDown == bPrevDown)
		{
			return;
		}
		bPrevDown = bIsDown;
	}

	const bool bMatches = (InputStateChangeToListenFor == EGASCourseInputStateChange::Down) ? bIsDown : !bIsDown;
	if (!bMatches)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())   // missing in the current code
	{
		OnInputStateChangeDelegate.Broadcast();
	}

	if (bTriggerOnce)
	{
		bTaskTriggered = true;
		EndTask();
	}
	
	bInitialCheckDone = true;
}
