// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/WeaponInventoryComponent/GASC_WeaponInventoryComponent.h"


// Sets default values for this component's properties
UGASC_WeaponInventoryComponent::UGASC_WeaponInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UGASC_WeaponInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

