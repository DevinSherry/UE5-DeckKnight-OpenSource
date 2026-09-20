// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/InputBuffer/GASC_InputBuffer_NotifyState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Game/Character/Components/InputBuffer/GASC_InputBufferComponent.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"


void UGASC_InputBuffer_NotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                float TotalDuration, const FAnimNotifyEventReference& EventReference)
{	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (AGASCoursePlayerCharacter* OwningCharacter = Cast<AGASCoursePlayerCharacter>(MeshComp->GetOwner()))
	{
		if (UGASC_InputBufferComponent* InputBufferComponent = OwningCharacter->GetInputBufferComponent())
		{
			if (bBlockInput)
			{
				for (FGameplayTag CategoryTag : InputCategoriesToBuffer.GetGameplayTagArray())
				{
					InputBufferComponent->BlockInputCategoryFromBuffer(CategoryTag);
				}
			}
			
			if (InputBufferOpenTimeoutOverride > 0.0f)
			{
				InputBufferComponent->SetInputBufferTimeout(InputBufferOpenTimeoutOverride);
			}
		}
	}
}

void UGASC_InputBuffer_NotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	float CurrentNotifyStatePercentage = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference);
	if (AGASCoursePlayerCharacter* OwningCharacter = Cast<AGASCoursePlayerCharacter>(MeshComp->GetOwner()))
	{
		if (UGASC_InputBufferComponent* InputBufferComponent = OwningCharacter->GetInputBufferComponent())
		{
			if (bBlockInput)
			{
				for (FGameplayTag CategoryTag : InputCategoriesToBuffer)
				{
					if (CurrentNotifyStatePercentage >= OpenInputBufferTimePercentage && OpenInputBufferTimePercentage != 1.0f)
					{
						if (!InputBufferComponent->IsInputBufferOpenForCategory(CategoryTag))
						{
							InputBufferComponent->OpenInputBuffer_ForCategory(CategoryTag);
							InputBufferComponent->ReleaseBlockInputCategoryFromBuffer(CategoryTag);
						}
					}
				}
			}
			else
			{
				for (FGameplayTag CategoryTag : InputCategoriesToBuffer)
				{
					if (!InputBufferComponent->IsInputBufferOpenForCategory(CategoryTag))
					{
						InputBufferComponent->OpenInputBuffer_ForCategory(CategoryTag);
					}
				}
			}
		}
	}
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UGASC_InputBuffer_NotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (AGASCoursePlayerCharacter* OwningCharacter = Cast<AGASCoursePlayerCharacter>(MeshComp->GetOwner()))
	{
		if (UGASC_InputBufferComponent* InputBufferComponent = OwningCharacter->GetInputBufferComponent())
		{
			for (FGameplayTag CategoryTag : InputCategoriesToBuffer)
			{
				InputBufferComponent->ReleaseBlockInputCategoryFromBuffer(CategoryTag);
				InputBufferComponent->CloseInputBuffer_ForCategory(CategoryTag);
			}
		}
	}
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UGASC_InputBuffer_NotifyState::GetNotifyName_Implementation() const
{
	FString NotifyName = TEXT("Input Buffer");

	NotifyName.ReplaceInline(TEXT("AnimNotifyState_"), TEXT(""), ESearchCase::CaseSensitive);
	
	FString Suffix = FString::Format(TEXT(" -> Open Input Buffer at: {0}%"), {FString::FromInt(OpenInputBufferTimePercentage * 100.0f)});
	NotifyName.Append(Suffix);
	
	return NotifyName;
}
