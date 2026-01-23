// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayEffect/Components/SendGameplayEventEffectComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"

void USendGameplayEventEffectComponent::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec,
                                                                FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetContext();
	const FGameplayEffectContext* Context = ContextHandle.Get();
	if (!Context)
	{
		return;
	}

	const TArray<TWeakObjectPtr<AActor>>& Actors = Context->GetActors();
	AActor* Instigator = Context->GetInstigator();

	UAbilitySystemComponent* InstigatorASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);

	FGameplayTagContainer InstigatorTags =
		InstigatorASC ? InstigatorASC->GetOwnedGameplayTags() : FGameplayTagContainer();

	// ---- Send event to targets ----
	if (TargetEventTag.IsValid() && Actors.Num() > 0)
	{
		FGameplayEventData EventData;
		EventData.EventTag = TargetEventTag;
		EventData.ContextHandle = ContextHandle;
		EventData.Instigator = Instigator;
		EventData.InstigatorTags = InstigatorTags;

		for (const TWeakObjectPtr<AActor>& Actor : Actors)
		{
			if (AActor* TargetActor = Actor.Get())
			{
				if (UAbilitySystemComponent* TargetASC =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
				{
					TargetASC->HandleGameplayEvent(TargetEventTag, &EventData);
				}
			}
		}
	}

	// ---- Send event to instigator ----
	if (Instigator && InstigatorEventTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator))
		{
			FGameplayEventData EventData;
			EventData.EventTag = InstigatorEventTag;
			EventData.ContextHandle = ContextHandle;
			EventData.Instigator = Instigator;
			EventData.InstigatorTags = InstigatorTags;

			ASC->HandleGameplayEvent(InstigatorEventTag, &EventData);
		}
	}
}
