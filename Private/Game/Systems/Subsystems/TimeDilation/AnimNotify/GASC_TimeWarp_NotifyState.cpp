// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Subsystems/TimeDilation/AnimNotify/GASC_TimeWarp_NotifyState.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Kismet/KismetMathLibrary.h"


UGASC_TimeWarp_NotifyState::UGASC_TimeWarp_NotifyState()
{
	TimeWarpCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	TimeWarpCurve.EditorCurveData.AddKey(1.0f, 1.0f);

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor::Red;
#endif 
}

void UGASC_TimeWarp_NotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (TimeWarpCurve.EditorCurveData.Keys.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty TimeWarp Curve used: %s - Consider removing. Will not process"), *GetPathNameSafe(this));
		return;
	}

	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation);
	if (!AnimMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("TimeWarp Notify State only works with AnimMontages: %s"), *GetPathNameSafe(this));
		return;
	}

	// Get or create state for this mesh component
	FTimeWarpState& State = ActiveStates.FindOrAdd(MeshComp);
	State.Duration = TotalDuration;
	State.bTimeWarpEnabled = true;
	State.AnimInstance = AnimInstance;
	State.AnimMontage = AnimMontage;
	State.DefaultPlayRate = AnimInstance->Montage_GetPlayRate(AnimMontage);

	// Bind delegate if not already bound for this specific instance
	if (!AnimInstance->OnMontageBlendingOut.IsAlreadyBound(this, &UGASC_TimeWarp_NotifyState::OnMontageBlendingOut))
	{
		AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UGASC_TimeWarp_NotifyState::OnMontageBlendingOut);
	}
}

void UGASC_TimeWarp_NotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	FTimeWarpState* State = ActiveStates.Find(MeshComp);
	if (!State || !State->bTimeWarpEnabled || !State->AnimInstance.IsValid())
	{
		return;
	}

	UAnimInstance* AnimInstance = State->AnimInstance.Get();
	UAnimMontage* AnimMontage = State->AnimMontage.Get();

	if (!AnimMontage || !AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		// Montage stopped playing, cleanup
		if (AnimMontage)
		{
			AnimInstance->Montage_SetPlayRate(AnimMontage, State->DefaultPlayRate);
		}
		State->bTimeWarpEnabled = false;
		return;
	}

	// Get curve and validate
	FRichCurve* RichCurve = TimeWarpCurve.GetRichCurve();
	if (!RichCurve)
	{
		UE_LOG(LogTemp, Warning, TEXT("TimeWarp Curve is invalid: %s"), *GetPathNameSafe(this));
		State->bTimeWarpEnabled = false;
		return;
	}

	float CurveMinTime = 0.0f;
	float CurveMaxTime = 0.0f;
	RichCurve->GetTimeRange(CurveMinTime, CurveMaxTime);

	float CurrentAnimationStateTime = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTime(EventReference);
	float AdjustedAnimationStateTime = UKismetMathLibrary::MapRangeClamped(CurrentAnimationStateTime, 0.0f, State->Duration, 0.0f, CurveMaxTime);
	float CurveValue = RichCurve->Eval(AdjustedAnimationStateTime);
	
	AnimInstance->Montage_SetPlayRate(AnimMontage, CurveValue);
}

void UGASC_TimeWarp_NotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	FTimeWarpState* State = ActiveStates.Find(MeshComp);
	if (!State)
	{
		return;
	}

	if (State->bTimeWarpEnabled && State->AnimInstance.IsValid() && State->AnimMontage.IsValid())
	{
		UAnimInstance* AnimInstance = State->AnimInstance.Get();
		UAnimMontage* AnimMontage = State->AnimMontage.Get();

		if (AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			AnimInstance->Montage_SetPlayRate(AnimMontage, State->DefaultPlayRate);
		}

		// Always unbind delegate in NotifyEnd
		if (AnimInstance->OnMontageBlendingOut.IsAlreadyBound(this, &UGASC_TimeWarp_NotifyState::OnMontageBlendingOut))
		{
			AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &UGASC_TimeWarp_NotifyState::OnMontageBlendingOut);
		}
	}

	// Clean up state
	State->bTimeWarpEnabled = false;
	ActiveStates.Remove(MeshComp);
}

void UGASC_TimeWarp_NotifyState::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// Find and cleanup any active states using this montage
	for (auto It = ActiveStates.CreateIterator(); It; ++It)
	{
		FTimeWarpState& State = It->Value;
		if (State.AnimMontage.Get() == Montage && State.bTimeWarpEnabled && State.AnimInstance.IsValid())
		{
			UAnimInstance* AnimInstance = State.AnimInstance.Get();
			AnimInstance->Montage_SetPlayRate(Montage, State.DefaultPlayRate);
			State.bTimeWarpEnabled = false;

			// Remove from map since montage is done
			It.RemoveCurrent();
		}
	}
}
