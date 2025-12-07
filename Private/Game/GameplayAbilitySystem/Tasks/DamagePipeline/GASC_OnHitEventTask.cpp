#include "Game/GameplayAbilitySystem/Tasks/DamagePipeline/GASC_OnHitEventTask.h"
#include "AbilitySystemComponent.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "Engine/World.h"

UGASC_OnHitEventTask::UGASC_OnHitEventTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGASC_OnHitEventTask* UGASC_OnHitEventTask::WaitOnHitEvent(
	UGameplayAbility* OwningAbility,
	AActor* OptionalExternalTarget,
	EHitEventType HitEvent)
{
	UGASC_OnHitEventTask* Task = NewAbilityTask<UGASC_OnHitEventTask>(OwningAbility);
	Task->HitEventToListenFor = HitEvent;

	// Default target is the ability avatar
	Task->TargetActor = OwningAbility->GetAvatarActorFromActorInfo();

	// Optionally override
	Task->SetExternalTarget(OptionalExternalTarget);

	return Task;
}

void UGASC_OnHitEventTask::SetExternalTarget(AActor* Actor)
{
	if (Actor)
	{
		bUseExternalTarget = true;
		OptionalExternalTarget = Actor;
	}
}

AActor* UGASC_OnHitEventTask::GetTarget() const
{
	if (bUseExternalTarget && OptionalExternalTarget.IsValid())
	{
		return OptionalExternalTarget.Get();
	}
	return TargetActor.Get();
}

/* -----------------------
   Internal Handlers
------------------------*/

void UGASC_OnHitEventTask::HandleHitApplied(const FHitContext& HitContext)
{
	AActor* Target = GetTarget();
	if (ShouldBroadcastAbilityTaskDelegates() &&
	    HitContext.HitInstigator.Get() == Target)
	{
		OnHitDelegate.Broadcast(HitContext);
	}
}

void UGASC_OnHitEventTask::HandleHitReceived(const FHitContext& HitContext)
{
	AActor* Target = GetTarget();
	if (ShouldBroadcastAbilityTaskDelegates() &&
	    HitContext.HitTarget.Get() == Target)
	{
		OnHitDelegate.Broadcast(HitContext);
	}
}

/* -----------------------
   Activation
------------------------*/

void UGASC_OnHitEventTask::Activate()
{
	Super::Activate();

	AActor* Target = GetTarget();
	if (!IsValid(Target))
	{
		EndTask();
		return;
	}

	if (UGASC_DamagePipelineSubsystem* Pipeline = GetWorld()->GetSubsystem<UGASC_DamagePipelineSubsystem>())
	{
		// Create local callback delegate
		FOnHitApplied_Event LocalDelegate;

		if (HitEventToListenFor == EHitEventType::OnHitApplied)
		{
			LocalDelegate.BindUFunction(this, FName("HandleHitApplied"));
		}
		else
		{
			LocalDelegate.BindUFunction(this, FName("HandleHitReceived"));
		}

		// Register into unified system
		Pipeline->RegisterActorOnHit(this, HitEventToListenFor, LocalDelegate);
	}

	ReadyForActivation();
}

/* -----------------------
   Destroy
------------------------*/

void UGASC_OnHitEventTask::OnDestroy(bool AbilityEnded)
{
	if (UGASC_DamagePipelineSubsystem* Pipeline = GetWorld()->GetSubsystem<UGASC_DamagePipelineSubsystem>())
	{
		// Must rebuild the same delegate to unregister
		FOnHitApplied_Event LocalDelegate;

		if (HitEventToListenFor == EHitEventType::OnHitApplied)
		{
			LocalDelegate.BindUFunction(this, FName("HandleHitApplied"));
		}
		else
		{
			LocalDelegate.BindUFunction(this, FName("HandleHitReceived"));
		}

		Pipeline->UnRegisterActorOnHit(this, HitEventToListenFor, LocalDelegate);
	}

	Super::OnDestroy(AbilityEnded);
}
