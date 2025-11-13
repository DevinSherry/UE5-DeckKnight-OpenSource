// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GASC_TimeWarp_NotifyState.generated.h"

/**
 * @class UGASC_TimeWarp_NotifyState
 * @brief A notify state class used for time warp behavior during animations.
 *
 * This class is typically used in animation systems to handle specific logic
 * related to time manipulation or "time warp" effects during a defined state
 * of an animation sequence.
 *
 * The UGASC_TimeWarp_NotifyState class allows developers to define custom
 * functionality for pausing, slowing down, speeding up, or otherwise altering
 * time behavior during particular animation timelines.
 *
 * Key Responsibilities:
 * - Determine when the time warp should be triggered during an animation.
 * - Apply custom logic to manipulate animation speed or behavior.
 * - Cleanup or reset any changes to the time warp state as needed.
 *
 * This notify state is designed to integrate seamlessly with the Unreal Engine
 * animation framework.
 */
UCLASS()
class GASCOURSE_API UGASC_TimeWarp_NotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	UGASC_TimeWarp_NotifyState();
	
	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference);
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference);
	virtual void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference);
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Curve")
	FRuntimeFloatCurve TimeWarpCurve;

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

private:

	// Store per-component state to handle multiple instances
	struct FTimeWarpState
	{
		float Duration = 0.0f;
		float DefaultPlayRate = 1.0f;
		bool bTimeWarpEnabled = false;
		TWeakObjectPtr<UAnimInstance> AnimInstance = nullptr;
		TWeakObjectPtr<UAnimMontage> AnimMontage = nullptr;
	};
	
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FTimeWarpState> ActiveStates;

};
