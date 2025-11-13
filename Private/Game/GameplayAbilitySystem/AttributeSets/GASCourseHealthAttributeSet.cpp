// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseHealthAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Game/Systems/CardEnergy/GASCourseCardEnergyExecution.h"

UGASCourseHealthAttributeSet::UGASCourseHealthAttributeSet()
{

}

void UGASCourseHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if(Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(CurrentHealth, MaxHealth, NewValue, GetCurrentHealthAttribute());
	}

	if(Attribute == GetStatusDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}

	if(Attribute == GetAllDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	
	if(Attribute == GetElementalDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}

	if(Attribute == GetPhysicalDamageHealingCoefficientAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}

	if(Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 1.0f);
	}
	if(Attribute == GetCriticalDamageMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}
	
}

void UGASCourseHealthAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UGASCourseHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                                       float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
}

void UGASCourseHealthAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UGASCourseHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AGASCourseCharacter* TargetCharacter = nullptr;
	UAbilitySystemComponent* TargetAbilitySystemComponent = nullptr;

	AActor* SourceActor = nullptr;
	AGASCourseCharacter* SourceCharacter = nullptr;
	UAbilitySystemComponent* SourceAbilitySystemComponent = nullptr;
	
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AGASCourseCharacter>(TargetActor);
		TargetAbilitySystemComponent = TargetCharacter->GetAbilitySystemComponent();
	}

	if (Data.EffectSpec.GetContext().IsValid() && Data.EffectSpec.GetContext().GetInstigator() && Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
	{
		SourceActor = Data.EffectSpec.GetContext().GetInstigator();
		SourceCharacter = Cast<AGASCourseCharacter>(SourceActor);
		SourceAbilitySystemComponent = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
	}

	if(Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		bool bIsAlive = TargetCharacter->IsCharacterAlive();
		
		const float HealthBeforeDamage = CurrentHealth.GetCurrentValue();
		const float NewHealth = CurrentHealth.GetCurrentValue() - LocalDamage;
		
		SetCurrentHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
	
		if(NewHealth <= 0.0f && bIsAlive)
		{
			TargetCharacter->SetCharacterDead(true);
			
			FGameplayEventData OnDeathPayload;
			OnDeathPayload.EventTag = Event_OnDeath;
			OnDeathPayload.Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
			
			FGameplayTagContainer TargetTags;
			TargetAbilitySystemComponent->GetOwnedGameplayTags(TargetTags);
			OnDeathPayload.TargetTags = TargetTags;

			FGameplayTagContainer SourceTags;
			SourceAbilitySystemComponent->GetOwnedGameplayTags(SourceTags);
			OnDeathPayload.InstigatorTags = SourceTags;
			
			OnDeathPayload.Target = GetOwningActor();
			OnDeathPayload.ContextHandle = Data.EffectSpec.GetContext();
			OnDeathPayload.EventMagnitude = LocalDamage;
			TargetAbilitySystemComponent->HandleGameplayEvent(Event_OnDeath, &OnDeathPayload);
			SourceAbilitySystemComponent->HandleGameplayEvent(Event_OnDeathDealt, &OnDeathPayload);
			

			TSubclassOf<UGameplayEffectExecutionCalculation> CardResourceExecutionClass;
			if (AbilitySystemSettings)
			{
				CardResourceExecutionClass = AbilitySystemSettings->CardResourceExecution;
				if (!CardResourceExecutionClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("Health Calculation is not valid!"));
					return;
				}
			}
		}
	}
	//Passive Healing Event
	if(Data.EvaluatedData.Attribute == GetIncomingHealingAttribute() && CurrentHealth.GetCurrentValue() != MaxHealth.GetCurrentValue())
	{
		const float LocalIncomingHealing = GetIncomingHealing();
		SetIncomingHealing(0.0f);
		
		float NewCurrentHealth = GetCurrentHealth() + LocalIncomingHealing;
		SetCurrentHealth(FMath::Clamp(NewCurrentHealth, 0.0f, GetMaxHealth()));

		FGameplayEventData OnHealingPayload;
		OnHealingPayload.EventTag = Event_Gameplay_OnHealing;
		OnHealingPayload.Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
		OnHealingPayload.Target = GetOwningActor();
		OnHealingPayload.ContextHandle = Data.EffectSpec.GetContext();
		OnHealingPayload.EventMagnitude = LocalIncomingHealing;
		TargetAbilitySystemComponent->HandleGameplayEvent(Event_Gameplay_OnHealing, &OnHealingPayload);
	}

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.0f, GetMaxHealth()));
	}
}
