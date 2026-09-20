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
	AActor* Avatar = GetAvatarActorFromActorInfo();
	const AActor* Instigator = Cast<AActor>(CurrentEventData.Instigator);

	if (!Avatar || !Instigator)
	{
		return;
	}

	FVector ToInstigator = Instigator->GetActorLocation() - Avatar->GetActorLocation();
	ToInstigator.Z = 0.f; // keep planar
	ToInstigator.Normalize();

	FRotator Wanted = ToInstigator.Rotation();

	if (APawn* Pawn = Cast<APawn>(Avatar))
	{
		Avatar->GetRootComponent()->SetWorldRotation(Wanted);
	}

	// Fallback if no controller exists (rare)
	Avatar->SetActorRotation(Wanted);
}
