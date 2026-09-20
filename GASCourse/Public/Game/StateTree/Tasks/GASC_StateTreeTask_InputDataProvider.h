// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnhancedInputComponent.h"
#include "StateTreeTaskBase.h"
#include "GASC_StateTreeTask_InputDataProvider.generated.h"

/**
 * @class UInputAction
 * @brief Represents an input action definition for handling user interactions.
 *
 * This class is utilized to define and encapsulate the details of input actions
 * that can be performed by the user. It serves as a blueprint for triggering
 * specific responses when associated input events occur.
 *
 * Key features of the UInputAction include the ability to bind user interactions,
 * categorize different action types, and manage input-related metadata within
 * an application.
 *
 * The UInputAction class is commonly used in input handling systems for games
 * or interactive software to manage a structured framework of user controls.
 *
 * It is designed to be extensible to facilitate additional custom behaviors
 * related to event-driven user input processing.
 */

class UInputAction;

USTRUCT(Blueprintable)
struct FInstanceDataInputProvider
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Controller|Input")
	TObjectPtr<UInputAction> InputAction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Output")
	ETriggerEvent InputTriggerEvent = ETriggerEvent::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Output")
	FInputActionValue InputActionValue;
};

USTRUCT(meta=(DisplayName="GASC Input Data Provider"))
struct FInputDataProviderTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	FInputDataProviderTask();

	using FInstanceDataType = FInstanceDataInputProvider;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataInputProvider::StaticStruct(); }

	// === Task lifecycle ===
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	void UpdateInputDataFromPlayerInput(const FStateTreeExecutionContext& Context) const;
	
};