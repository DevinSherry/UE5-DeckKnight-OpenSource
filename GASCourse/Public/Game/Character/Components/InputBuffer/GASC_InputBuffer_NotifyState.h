// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GASC_InputBuffer_NotifyState.generated.h"

/**
 * UGASC_InputBuffer_NotifyState is an animation notify state class used to handle input buffering mechanics
 * during animation sequences. This class enables gameplay mechanics to open and close input buffering
 * based on animation notify states.
 */
UCLASS()
class GASCOURSE_API UGASC_InputBuffer_NotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	
	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference);
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference);
	virtual void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference);

	virtual FString GetNotifyName_Implementation() const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer", meta=(Categories = "InputBuffer.Category"))
	FGameplayTagContainer InputCategoriesToBuffer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	bool bBlockInput = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input Buffer", meta=(ClampMin=0.0f, ClampMax=1.0f, EditCondition = "bBlockInput", EditConditionHides))
	float OpenInputBufferTimePercentage = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	float InputBufferOpenTimeoutOverride = 0.0f;
};
