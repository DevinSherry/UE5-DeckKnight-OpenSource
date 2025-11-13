// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "GASCourse/GASCourseCharacter.h"

DEFINE_LOG_CATEGORY(LOG_GASC_DamagePipelineSubsystem);

bool UGASC_DamagePipelineSubsystem::OnHitEvent(const FHitContext& HitContext)
{
	if (!HitContext.HitTarget.IsValid())
	{
		UE_LOGFMT(LogTemp, Error, "Hit Target is invalid. On Hit Event will not be broadcasted.");
		return false;
	}
	if (!HitContext.HitInstigator.IsValid())
	{
		UE_LOGFMT(LogTemp, Error, "Hit Instigator is invalid. On Hit Event will not be broadcasted.");
		return false;
	}

	if (AGASCourseCharacter* TargetCharacter = Cast<AGASCourseCharacter>(HitContext.HitTarget.Get()))
	{
		TargetCharacter->OnHitReceivedDelegateCallback.Broadcast(HitContext);
	}

	if (AGASCourseCharacter* InstigatorCharacter = Cast<AGASCourseCharacter>(HitContext.HitInstigator.Get()))
	{
		InstigatorCharacter->OnHitAppliedDelegateCallback.Broadcast(HitContext);
	}

	OnHitAppliedDelegateCallback.Broadcast(HitContext);
	OnHitReceivedDelegateCallback.Broadcast(HitContext);
	
	return true;
}

bool UGASC_DamagePipelineSubsystem::SendHitRequest(const FHitContext& HitContext)
{
	return OnHitEvent(HitContext);
}

void UGASC_DamagePipelineSubsystem::CallbackTester(FOnHitApplied_Event callback)
{
	if (!callback.IsBound())
		return;

	FScriptDelegate ScriptDelegate;
	ScriptDelegate.BindUFunction(this, FName("Internal_ForwardOnHitApplied"));
	OnHitAppliedDelegateCallback.Add(ScriptDelegate);

	ForwardedCallback = callback;
}

void UGASC_DamagePipelineSubsystem::Internal_ForwardOnHitApplied(const FHitContext& HitContext,TWeakObjectPtr<AActor> Listener)
{
	if (ForwardedCallback.IsBound())
	{
		ForwardedCallback.Execute(HitContext);
	}
}
