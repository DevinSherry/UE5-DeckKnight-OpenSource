// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"

// Sets default values for this component's properties
UDeckManagerComponent::UDeckManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UDeckManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

bool UDeckManagerComponent::ActivateCard(UCardDataAsset* CardToActivate)
{
	bool bCardActivated = false;

	if (!CardToActivate)
	{
		UE_LOG(LogTemp, Error, TEXT("Card to activate is NULL"));
		return bCardActivated;
	}

	CardToActivate->ActivateCard(GetOwner());
	
	return bCardActivated;
}

