// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyRef.h"
#include "StructUtils/PropertyBag.h" // FInstancedPropertyBag
#include "GASC_STTask_CopyProperty.generated.h"

USTRUCT()
struct FCopyPropertyTaskInstanceData
{
	GENERATED_BODY()

	// Source bag (you can fill it elsewhere, or bind it from context/another parameter)
	UPROPERTY(EditAnywhere, Category="Parameters")
	FInstancedPropertyBag NewPropertyValue;

	// Target parameter reference (must be bound to a parameter of type FInstancedPropertyBag)
	// Note: RefType must be set and structs use full path name. [1](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/StateTreeModule/FStateTreePropertyRef)
	UPROPERTY(EditAnywhere, Category="Input", meta=(RefType="/* full path to FInstancedPropertyBag struct */"))
	FStateTreePropertyRef TargetPropertyRef;
};

USTRUCT(meta=(DisplayName="GASC Copy Parameter"))
struct FCopyBagToParamTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FCopyPropertyTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override
	{
		FInstanceDataType& Data = Context.GetInstanceData<FInstanceDataType>(*this);

		if (FInstancedPropertyBag* TargetBag = Data.TargetPropertyRef.GetMutablePtr<FInstancedPropertyBag>(Context))
		{
			// Generic copy by property name, no per-type ifs. [2](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/FInstancedPropertyBag/CopyMatchingValuesByName)
			TargetBag->CopyMatchingValuesByName(Data.NewPropertyValue, TOptional<TConstArrayView<FName>>());
			return EStateTreeRunStatus::Succeeded;
		}

		return EStateTreeRunStatus::Failed;
	}
};

