// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "GASCourse_WaitButtonDown.generated.h"

class UEnhancedPlayerInput;
class UInputAction;

UENUM(BlueprintType)
enum class EGASCourseInputStateChange : uint8
{
	None UMETA(Hidden),
	Up,
	Down,
};
ENUM_CLASS_FLAGS(EGASCourseInputStateChange)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGASCourseWaitInputStateChangeDelegate);

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASCourse_WaitInputStateChange : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FGASCourseWaitInputStateChangeDelegate OnInputStateChangeDelegate;
	
protected:	
	virtual void Activate() override;
	
public:
	
	virtual void OnDestroy(bool AbilityEnded) override;
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGASCourse_WaitInputStateChange* WaitButtonStateChanged(UGameplayAbility* OwningAbility, UInputAction* InputAction, 
		EGASCourseInputStateChange InputStateChangeToListen = EGASCourseInputStateChange::Up, bool bTriggerOnce = true);
	
	virtual void TickTask(float DeltaTime) override;
	
private:
	
	UPROPERTY()
	TObjectPtr<UInputAction> InputActionToListen;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;
	
	UPROPERTY()
	EGASCourseInputStateChange InputStateChangeToListenFor;
	
	UPROPERTY()
	TWeakObjectPtr<UEnhancedPlayerInput> EnhancedPlayerInput;
	
	UPROPERTY()
	bool bTaskTriggered = false;
	
	UPROPERTY()
	bool bTriggerOnce = true;
	
	uint8 bHasPrevState : 1 = false;
	uint8 bPrevDown : 1 = false;
	
	UPROPERTY()
	bool bInitialCheckDone = false;

	
	void CheckInputState(bool bInitialCheck = false);
};
