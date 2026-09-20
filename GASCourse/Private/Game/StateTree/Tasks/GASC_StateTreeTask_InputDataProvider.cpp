// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Tasks/GASC_StateTreeTask_InputDataProvider.h"
#include "StateTreeExecutionContext.h"
#include "EnhancedPlayerInput.h"

FInputDataProviderTask::FInputDataProviderTask()
{
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bShouldStateChangeOnReselect = false;
#endif
}

EStateTreeRunStatus FInputDataProviderTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const EStateTreeRunStatus RunStatus = FStateTreeTaskCommonBase::EnterState(Context, Transition);
	UpdateInputDataFromPlayerInput(Context);

	return RunStatus;
}

void FInputDataProviderTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{

	UpdateInputDataFromPlayerInput(Context);
	FStateTreeTaskCommonBase::ExitState(Context, Transition);
}

EStateTreeRunStatus FInputDataProviderTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const EStateTreeRunStatus RunStatus = FStateTreeTaskCommonBase::Tick(Context, DeltaTime);
	UpdateInputDataFromPlayerInput(Context);

	return RunStatus;
}

void FInputDataProviderTask::UpdateInputDataFromPlayerInput(const FStateTreeExecutionContext& Context) const
{
	FInstanceDataInputProvider& InstanceData = Context.GetInstanceData(*this);
	const UInputAction* InputAction = InstanceData.InputAction;
	if (!InputAction)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(Context.GetOwner());
	if (!PlayerController)
	{
		return;
	}

	const UEnhancedPlayerInput* PlayerInput = Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput);
	if (!PlayerInput)
	{
		return;
	}

	const FInputActionInstance* ActionInstance = PlayerInput->FindActionInstanceData(InputAction);
	if (!ActionInstance)
	{
		return;
	}

	InstanceData.InputActionValue = ActionInstance->GetValue();
	InstanceData.InputTriggerEvent = ActionInstance->GetTriggerEvent();
	
#if UE_EDITOR
	
	const FString TriggerEventString =
	StaticEnum<ETriggerEvent>()->GetNameStringByValue(
		static_cast<int64>(InstanceData.InputTriggerEvent));
	
	FString InputValueString;

	switch (InstanceData.InputActionValue.GetValueType())
	{
	case EInputActionValueType::Boolean:
		InputValueString = InstanceData.InputActionValue.Get<bool>()
			? TEXT("True")
			: TEXT("False");
		break;

	case EInputActionValueType::Axis1D:
		InputValueString = FString::SanitizeFloat(
			InstanceData.InputActionValue.Get<float>());
		break;

	case EInputActionValueType::Axis2D:
		InputValueString =
			InstanceData.InputActionValue.Get<FVector2D>().ToString();
		break;

	case EInputActionValueType::Axis3D:
		InputValueString =
			InstanceData.InputActionValue.Get<FVector>().ToString();
		break;

	default:
		InputValueString = TEXT("None");
		break;
	}
	
	UE_LOGFMT(LogTemp, Warning, "Input Action Value: {0} | Input Trigger Event: {1}", InputValueString, TriggerEventString);
#endif
	
}
