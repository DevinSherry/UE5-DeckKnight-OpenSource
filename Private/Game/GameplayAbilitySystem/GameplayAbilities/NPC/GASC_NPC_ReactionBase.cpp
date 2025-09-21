// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayAbilities/NPC/GASC_NPC_ReactionBase.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

void UGASC_NPC_ReactionBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (bFaceInstigatorOnReactionActivated)
	{
		FaceInstigator();
	}
}

void UGASC_NPC_ReactionBase::FaceInstigator()
{
	if (!CurrentEventData.Instigator)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentEventData.Instigator is NULL. Will not face instigator"));
		return;	
	}

	FVector InstigatorLocation = CurrentEventData.Instigator->GetActorLocation();
	FVector TargetLocation = CurrentEventData.Target ? CurrentEventData.Target->GetActorLocation() : GetAvatarActorFromActorInfo()->GetActorLocation();
	FVector DirectionVector = UKismetMathLibrary::GetDirectionUnitVector(TargetLocation, InstigatorLocation);
	FRotator Rotation = UKismetMathLibrary::MakeRotFromX(DirectionVector);
	GetAvatarActorFromActorInfo()->SetActorRotation(Rotation);
}
