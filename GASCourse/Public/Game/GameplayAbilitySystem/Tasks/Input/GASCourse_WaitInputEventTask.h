// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "EnhancedInputComponent.h"
#include "GASCourse_WaitInputEventTask.generated.h"

UENUM(BlueprintType, meta = (BitFlags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EGASCourseInputTriggerEventMask : uint8
{
	None      = 0 UMETA(Hidden),
	Started   = 1 << 0,
	Ongoing   = 1 << 1,
	Triggered = 1 << 2,
	Canceled  = 1 << 3,
	Completed = 1 << 4,
};
ENUM_CLASS_FLAGS(EGASCourseInputTriggerEventMask)


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGASCourseWaitInputEventDelegate, const ETriggerEvent, InputEvent);

/**
 * @class UGASCourse_WaitInputEventTask
 *
 * @brief A task class designed to wait for and handle specific input events in the context
 *        of Gameplay Ability System.
 *
 * This class provides functionality to wait for predefined input events during gameplay and
 * execute associated behavior or transitions when the event is triggered. It is typically
 * utilized in the Unreal Engine's Gameplay Ability System pipeline to synchronize abilities
 * with player input.
 *
 * @details
 * - The task listens for player input events such as key presses, mouse clicks, or controller actions.
 * - Triggers custom logic or transitions upon detecting the specified input event.
 * - Can be customized or extended for various event-driven gameplay scenarios.
 *
 * Key Features:
 * - Integration with the Gameplay Ability System for seamless ability execution.
 * - Customizable input event subscriptions and handlers.
 * - Support for event-specific callbacks or triggers during runtime.
 *
 * Usage Scenarios:
 * - Waiting for player confirmation input before progressing an ability.
 * - Listening for specific key combinations or controller actions to trigger abilities.
 * - Synchronizing gameplay logic with real-time player interactions.
 *
 * Requirements:
 * - This class assumes a properly set up input binding system within the game project.
 * - The owning ability and its respective GameplayAbilityTask must be correctly initialized.
 *
 * Notes:
 * - Ensure the input event being monitored is adequately mapped within the project's input configuration.
 */
UCLASS()
class GASCOURSE_API UGASCourse_WaitInputEventTask : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FGASCourseWaitInputEventDelegate OnInputEvent;
	
	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;
	
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGASCourse_WaitInputEventTask* WaitInputEvent(UGameplayAbility* OwningAbility, UInputAction* InputAction,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/GASCourse.EGASCourseInputTriggerEventMask"))
		int32 InputTriggerEventMask = 0, bool bTriggerOnce = true);
	
	static bool HasFlag(int32 Mask, EGASCourseInputTriggerEventMask Flags);
	
private:
	
	UPROPERTY()
	int32 InputEventTaskFlags = 0;
	
	UPROPERTY()
	bool bTriggerOnce = true;
	
	UPROPERTY()
	TObjectPtr<UInputAction> InputActionToListen;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;
	
	TArray<uint32> InputBindingHandles;

	void RemoveInputBindings();
	void BindInputEvent(UEnhancedInputComponent& InputComponent, EGASCourseInputTriggerEventMask EventMask, ETriggerEvent TriggerEvent);
	
	void RespondToEvent(ETriggerEvent InputEvent);
};
