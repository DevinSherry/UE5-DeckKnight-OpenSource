// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/Input/GASCourse_WaitInputEventTask.h"

UGASCourse_WaitInputEventTask::UGASCourse_WaitInputEventTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UGASCourse_WaitInputEventTask::Activate()
{
	Super::Activate();

	AActor* Actor = TargetActor.Get();
	if (!IsValid(Actor))
	{
		EndTask();
		return;
	}

	UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(
		Actor->GetComponentByClass(UEnhancedInputComponent::StaticClass()));
	if (!InputComponent)
	{
		EndTask();
		return;
	}

	if (!InputActionToListen)
	{
		EndTask();
		return;
	}

	BindInputEvent(*InputComponent, EGASCourseInputTriggerEventMask::Started, ETriggerEvent::Started);
	BindInputEvent(*InputComponent, EGASCourseInputTriggerEventMask::Ongoing, ETriggerEvent::Ongoing);
	BindInputEvent(*InputComponent, EGASCourseInputTriggerEventMask::Triggered, ETriggerEvent::Triggered);
	BindInputEvent(*InputComponent, EGASCourseInputTriggerEventMask::Canceled, ETriggerEvent::Canceled);
	BindInputEvent(*InputComponent, EGASCourseInputTriggerEventMask::Completed, ETriggerEvent::Completed);
}

void UGASCourse_WaitInputEventTask::OnDestroy(bool AbilityEnded)
{
	RemoveInputBindings();
	Super::OnDestroy(AbilityEnded);
}

UGASCourse_WaitInputEventTask* UGASCourse_WaitInputEventTask::WaitInputEvent(UGameplayAbility* OwningAbility, UInputAction* InputAction,
	int32 InputTriggerEventMask, bool bTriggerOnce)
{
	UGASCourse_WaitInputEventTask* MyObj = NewAbilityTask<UGASCourse_WaitInputEventTask>(OwningAbility);
	MyObj->InputActionToListen = InputAction;
	MyObj->InputEventTaskFlags = InputTriggerEventMask;
	MyObj->bTriggerOnce = bTriggerOnce;
	
	// Default target is the ability avatar
	MyObj->TargetActor = OwningAbility->GetAvatarActorFromActorInfo();
	
	return MyObj;
}

bool UGASCourse_WaitInputEventTask::HasFlag(int32 Mask, EGASCourseInputTriggerEventMask Flags)
{
	return (Mask & static_cast<int32>(Flags)) != 0;
}

void UGASCourse_WaitInputEventTask::RemoveInputBindings()
{
	AActor* Actor = TargetActor.Get();
	if (!Actor)
	{
		InputBindingHandles.Reset();
		return;
	}

	UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(
		Actor->GetComponentByClass(UEnhancedInputComponent::StaticClass()));
	if (!InputComponent)
	{
		InputBindingHandles.Reset();
		return;
	}

	for (const uint32 BindingHandle : InputBindingHandles)
	{
		InputComponent->RemoveBindingByHandle(BindingHandle);
	}

	InputBindingHandles.Reset();
}

void UGASCourse_WaitInputEventTask::RespondToEvent(ETriggerEvent InputEvent)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnInputEvent.Broadcast(InputEvent);
		if (bTriggerOnce)
		{
			EndTask();
		}
	}
}

void UGASCourse_WaitInputEventTask::BindInputEvent(UEnhancedInputComponent& InputComponent, EGASCourseInputTriggerEventMask EventMask,
	ETriggerEvent TriggerEvent)
{
	if (!HasFlag(InputEventTaskFlags, EventMask))
	{
		return;
	}

	const TWeakObjectPtr<UGASCourse_WaitInputEventTask> WeakTask(this);

	FEnhancedInputActionEventBinding& Binding = InputComponent.BindActionValueLambda(
		InputActionToListen,
		TriggerEvent,
		[WeakTask, TriggerEvent](const FInputActionValue& Value)
		{
			if (!WeakTask.IsValid())
			{
				return;
			}

			WeakTask->RespondToEvent(TriggerEvent);
		});

	InputBindingHandles.Add(Binding.GetHandle());
}

