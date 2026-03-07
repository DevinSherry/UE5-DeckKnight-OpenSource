// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/Targeting/GASC_PlayerTargetingComponent.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UGASC_PlayerTargetingComponent::UGASC_PlayerTargetingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGASC_PlayerTargetingComponent::IsActorRecentMeleeTarget(const AActor* TargetActor) const
{
	return TargetActor ? RecentMeleeTargetActor == TargetActor : false;
}

// Called when the game starts
void UGASC_PlayerTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGASC_PlayerTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvalidateRecentMeleeTargetTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UGASC_PlayerTargetingComponent::ClearRecentMeleeTarget()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvalidateRecentMeleeTargetTimerHandle);
	}
	RecentMeleeTargetActor = nullptr;
}

void UGASC_PlayerTargetingComponent::UpdateRecentMeleeTarget(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	RecentMeleeTargetActor = TargetActor;

	if (UWorld* World = GetWorld())
	{
		// SetTimer with the same handle = reset/restart the timer
		World->GetTimerManager().SetTimer(
			InvalidateRecentMeleeTargetTimerHandle,
			this,
			&UGASC_PlayerTargetingComponent::ClearRecentMeleeTarget,
			RecentMeleeTargetRecentTimer,
			false
		);
	}
	
}

