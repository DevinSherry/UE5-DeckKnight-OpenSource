// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StateTreeTaskBase.h"
#include "GASC_StateTreeTask_BlockStateTreeTransition.generated.h"

class UStateTreeComponent;

/**
 * @enum EGASC_TaskDurationType
 *
 * Enumeration representing the duration type of a task in gameplay-related systems.
 */

UENUM(BlueprintType)
enum class EGASC_TaskDurationType : uint8
{
	HasDuration,
	OneFrame,
	StateDuration,
};

class UStateTreeComponent;

USTRUCT()
struct FGASC_StateTreeTask_BlockStateTreeTransition_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Duration")
	EGASC_TaskDurationType DurationType = EGASC_TaskDurationType::HasDuration;
	
	UPROPERTY(EditAnywhere, Category="Duration", meta=(EditCondition = "DurationType == EGASC_TaskDurationType::HasDuration", EditConditionHides))
	float Duration = 1.0f;
	
	UPROPERTY(EditAnywhere, Transient, Category="Output")
	bool bBlockTransition = false;
	
	FTimerHandle DurationTimerHandle;
};

USTRUCT(meta=(DisplayName="GASC Block State Tree Transition"))
struct FStateTreeTask_GASCBlockStateTreeTransition : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FStateTreeTask_GASCBlockStateTreeTransition();

	using FInstanceDataType = FGASC_StateTreeTask_BlockStateTreeTransition_InstanceData;

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