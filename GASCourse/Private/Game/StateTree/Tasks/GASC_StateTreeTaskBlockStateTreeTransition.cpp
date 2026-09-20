// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Tasks/GASC_StateTreeTask_BlockStateTreeTransition.h"
#include "StateTreeExecutionContext.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"
#include "Components/StateTreeComponent.h"
#include "StateTreeTaskBase.h"

// Namespaced rather than left as file statics: GASC_StateTreeTask_InputListener.cpp declares the
// same four helpers with the same signatures, and a unity build compiles both files into one
// translation unit, where two definitions of the same static function is a redefinition error.
namespace GASCourse::BlockStateTreeTransition
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
		if (AGASCoursePlayerCharacter* Char = BlockStateTreeTransition::GetPlayerChar(PC))
		{
			return Char->GetInputBufferComponent();
		}
		return nullptr;
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
namespace BlockStateTreeTransition = GASCourse::BlockStateTreeTransition;


FStateTreeTask_GASCBlockStateTreeTransition::FStateTreeTask_GASCBlockStateTreeTransition()
{
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FStateTreeTask_GASCBlockStateTreeTransition::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (!Context.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bBlockTransition = true;

	APlayerController* PC = BlockStateTreeTransition::GetPC(Context);
	if (!PC)
	{
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = PC->GetWorld();
	if (!World)
	{
		return EStateTreeRunStatus::Failed;
	}
	
	World->GetTimerManager().ClearTimer(Data.DurationTimerHandle);
	
	UStateTreeComponent* ST = BlockStateTreeTransition::GetStateTree(PC);
	UGASC_InputBufferComponent* InputBuffer = BlockStateTreeTransition::GetInputBuffer(PC);

	if (!ST || !InputBuffer)
	{
		return EStateTreeRunStatus::Failed;
	}
	
	switch (Data.DurationType)
	{
	case EGASC_TaskDurationType::HasDuration:
		{
			World->GetTimerManager().SetTimer(
				Data.DurationTimerHandle,
				[](){},
				Data.Duration,
				false
			);
			break;
		}

	case EGASC_TaskDurationType::OneFrame:
		{
			Data.DurationTimerHandle =
				World->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateLambda([](){})
				);

			break;
		}

	case EGASC_TaskDurationType::StateDuration:
		{
			break;
		}

	default:
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_GASCBlockStateTreeTransition::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (Context.IsValid())
	{
		FInstanceDataType& Data = Context.GetInstanceData(*this);

		Data.bBlockTransition = false;

		if (APlayerController* PC = BlockStateTreeTransition::GetPC(Context))
		{
			if (UWorld* World = PC->GetWorld())
			{
				World->GetTimerManager().ClearTimer(Data.DurationTimerHandle);
			}
		}
	}

	FStateTreeTaskCommonBase::ExitState(Context, Transition);
}

EStateTreeRunStatus FStateTreeTask_GASCBlockStateTreeTransition::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	// ✅ Timer-driven unblock ONLY
	if (Data.DurationType == EGASC_TaskDurationType::HasDuration ||
		Data.DurationType == EGASC_TaskDurationType::OneFrame)
	{
		APlayerController* PC = BlockStateTreeTransition::GetPC(Context);
		if (!PC)
		{
			return EStateTreeRunStatus::Failed;
		}

		UWorld* World = PC->GetWorld();
		if (!World)
		{
			return EStateTreeRunStatus::Failed;
		}

		// ✅ Poll timer (never rely on ExitState)
		if (!World->GetTimerManager().IsTimerActive(Data.DurationTimerHandle))
		{
			Data.bBlockTransition = false;
			World->GetTimerManager().ClearTimer(Data.DurationTimerHandle);
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeTask_GASCBlockStateTreeTransition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
                                                          const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("Allows to bind to output variable bBlockTransition to block state tree transitions.");
}
#endif
