// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/Tasks/DamagePipeline/GASC_OnDamageEventTask.h"

void UGASC_OnDamageEventTask::OnDamageEventReceived(const FResourceModificationContext& ResourceModificationContextContext)
{
	AActor* Target = GetTarget();
	if (ShouldBroadcastAbilityTaskDelegates() &&
		ResourceModificationContextContext.HitContext.HitTarget.Get() == Target)
	{
		OnDamageEvent.Broadcast(ResourceModificationContextContext);
	}
}

void UGASC_OnDamageEventTask::OnDamageEventApplied(const FResourceModificationContext& ResourceModificationContextContext)
{
	AActor* Target = GetTarget();
	if (ShouldBroadcastAbilityTaskDelegates() &&
		ResourceModificationContextContext.HitContext.HitInstigator.Get() == Target)
	{
		OnDamageEvent.Broadcast(ResourceModificationContextContext);
	}
}

void UGASC_OnDamageEventTask::Activate()
{
	Super::Activate();

	AActor* Target = GetTarget();
	if (!IsValid(Target))
	{
		EndTask();
		return;
	}

	if (UGASC_ResourcePipelineSubsystem* Pipeline = GetWorld()->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
	{
		// Create local callback delegate
		FOnDamageApplied_Event LocalDelegate;

		if (DamageEventToListenFor == EOnDamageEventType::OnDamageApplied)
		{
			LocalDelegate.BindUFunction(this, FName("OnDamageEventApplied"));
		}
		else
		{
			LocalDelegate.BindUFunction(this, FName("OnDamageEventReceived"));
		}
		
		// Register into unified system
		Pipeline->RegisterActorOnDamageEvent(this, DamageEventToListenFor, LocalDelegate);
	}

	ReadyForActivation();
}

void UGASC_OnDamageEventTask::OnDestroy(bool AbilityEnded)
{
	if (UGASC_ResourcePipelineSubsystem* Pipeline = GetWorld()->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
	{
		// Must rebuild the same delegate to unregister
		FOnDamageApplied_Event LocalDelegate;

		if (DamageEventToListenFor == EOnDamageEventType::OnDamageApplied)
		{
			LocalDelegate.BindUFunction(this, FName("OnDamageEventApplied"));
		}
		else
		{
			LocalDelegate.BindUFunction(this, FName("OnDamageEventReceived"));
		}

		Pipeline->UnRegisterActorOnDamageEvent(this, DamageEventToListenFor, LocalDelegate);
	}

	Super::OnDestroy(AbilityEnded);
}

UGASC_OnDamageEventTask* UGASC_OnDamageEventTask::WaitOnDamageEvent(UGameplayAbility* OwningAbility, AActor* OptionalExternalTarget,
	EOnDamageEventType DamageEventToListenFor)
{
	UGASC_OnDamageEventTask* Task = NewAbilityTask<UGASC_OnDamageEventTask>(OwningAbility);
	Task->DamageEventToListenFor = DamageEventToListenFor;

	// Default target is the ability avatar
	Task->TargetActor = OwningAbility->GetAvatarActorFromActorInfo();

	// Optionally override
	Task->SetExternalTarget(OptionalExternalTarget);
	
	return Task;
}

void UGASC_OnDamageEventTask::SetExternalTarget(AActor* Actor)
{
	if (Actor)
	{
		bUseExternalTarget = true;
		OptionalExternalTarget = Actor;
	}
}

AActor* UGASC_OnDamageEventTask::GetTarget() const
{
	if (bUseExternalTarget && OptionalExternalTarget.IsValid())
	{
		return OptionalExternalTarget.Get();
	}
	return TargetActor.Get();
}
