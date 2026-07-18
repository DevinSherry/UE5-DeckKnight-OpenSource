// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/Health/GASC_HealthComponent.h"
#include "MVVMGameSubsystem.h"
#include "MVVMSubsystem.h"
#include "Abilities/Tasks/AbilityTask_WaitAttributeChange.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "Game/HUD/ViewModels/Health/GASC_UVM_Health.h"
#include "GASCourse/GASCourseCharacter.h"

// Sets default values for this component's properties
UGASC_HealthComponent::UGASC_HealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UGASC_HealthComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeViewModel();
}

void UGASC_HealthComponent::InitializeHealthAttributes()
{
	if(AGASCourseCharacter* OwningCharacter = Cast<AGASCourseCharacter>(GetOwner()))
	{
		if(UAbilitySystemComponent* OwningASC = Cast<UAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent()))
		{
			if(const UGASCourseHealthAttributeSet* HealthAttributes = Cast<UGASCourseHealthAttributeSet>(OwningASC->GetAttributeSet(HealthAttributeSet)))
			{
				CurrentHealth = HealthAttributes->GetCurrentHealth();
				MaxHealth = HealthAttributes->GetMaxHealth();
			}
		}
	}
}

void UGASC_HealthComponent::InitializeViewModel()
{
	UMVVMGameSubsystem* ViewModelGameSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
	check(ViewModelGameSubsystem);

	UMVVMViewModelCollectionObject* GlobalViewModelCollection = ViewModelGameSubsystem->GetViewModelCollection();
	check(GlobalViewModelCollection);

	UGASC_UVM_Health* CharacterHealthViewModel = NewObject<UGASC_UVM_Health>();
	FMVVMViewModelContext CharacterHealthViewModelContext;
	CharacterHealthViewModelContext.ContextClass = CharacterHealthViewModelContextClass;
	CharacterHealthViewModelContext.ContextName = CharacterHealthContextName;
	if(CharacterHealthViewModelContext.IsValid())
	{
		if (GlobalViewModelCollection->FindViewModelInstance(CharacterHealthViewModelContext))
		{
			GlobalViewModelCollection->RemoveViewModel(CharacterHealthViewModelContext);
		}
		
		GlobalViewModelCollection->AddViewModelInstance(CharacterHealthViewModelContext, CharacterHealthViewModel);
		HealthViewModel = CharacterHealthViewModel;

		OnHealthViewModelInstantiated.Broadcast(HealthViewModel);
		HealthViewModelInstantiated(HealthViewModel);
	}
}

void UGASC_HealthComponent::HealthViewModelInstantiated_Implementation(UGASC_UVM_Health* InstantiatedViewModel)
{
	InitializeHealthAttributes();
}