// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/Utility/GASC_AbilityTickTask.h"


UGASC_AbilityTickTask::UGASC_AbilityTickTask(const FObjectInitializer& ObjectInitializer)
{
	bTickingTask = true;
}
 
UGASC_AbilityTickTask* UGASC_AbilityTickTask::AbilityTaskOnTick(UGameplayAbility* OwningAbility, FName TaskInstanceName)
{
	UGASC_AbilityTickTask* MyObj = NewAbilityTask<UGASC_AbilityTickTask>(OwningAbility, TaskInstanceName);
	return MyObj;
}
 
void UGASC_AbilityTickTask::Activate()
{
	Super::Activate();
}
 
void UGASC_AbilityTickTask::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnTick.Broadcast(DeltaTime);
	}
}