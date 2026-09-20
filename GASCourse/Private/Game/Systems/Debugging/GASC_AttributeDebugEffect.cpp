// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/GASC_AttributeDebugEffect.h"

UGASC_AttributeDebugEffect::UGASC_AttributeDebugEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to instant, we’ll override duration/period on the Spec if needed
	DurationPolicy = EGameplayEffectDurationType::Instant;
}
