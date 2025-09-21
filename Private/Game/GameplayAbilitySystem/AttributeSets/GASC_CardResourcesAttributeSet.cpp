// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASC_CardResourcesAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Kismet/KismetMathLibrary.h"

UGASC_CardResourcesAttributeSet::UGASC_CardResourcesAttributeSet()
{
}

void UGASC_CardResourcesAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaximumCardEnergyAttribute())
	{
		if(GetMaximumCardEnergy() == GetCurrentCardEnergy())
		{
			SetCurrentCardEnergyXP(0.0f);
        }
		if (NewValue < GetCurrentCardEnergy())
		{
			SetCurrentCardEnergy(NewValue);
		}
			
		//AdjustAttributeForMaxChange(CurrentCardEnergy, MaximumCardEnergy, NewValue, GetCurrentCardEnergyAttribute());
	}

	if(Attribute == GetCurrentCardEnergyAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, MaximumCardEnergy.GetCurrentValue());
		if (NewValue == GetMaximumCardEnergy())
		{
			SetCurrentCardEnergyXP(GetMaximumCardEnergyXP());
		}
		else if (NewValue < GetMaximumCardEnergy())
		{
			if (CurrentCardEnergyXP.GetCurrentValue() == GetMaximumCardEnergyXP())
			{
				SetCurrentCardEnergyXP(0.0f);
			}
		}
	}

	if(Attribute == GetActiveCardEnergyMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}

	if(Attribute == GetOnKillActiveCardEnergyMultiplierAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 10.0f);
	}
}

void UGASC_CardResourcesAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	
	if(Attribute == GetCurrentCardEnergyAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaximumCardEnergy());
	}
	if(Attribute == GetCurrentCardEnergyXPAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaximumCardEnergyXP());
	}
}

void UGASC_CardResourcesAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UGASC_CardResourcesAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UGASC_CardResourcesAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AGASCourseCharacter* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AGASCourseCharacter>(TargetActor);
	}

	if(Data.EvaluatedData.Attribute == GetIncomingCardEnergyXPAttribute())
	{
		const float LocalIncomingCardEnergyXP = GetIncomingCardEnergyXP();
		SetIncomingCardEnergyXP(0.0f);
		if (GetCurrentCardEnergy() == GetMaximumCardEnergy() && GetCurrentCardEnergyXP() == GetMaximumCardEnergyXP())
		{
			return;
		}

		const float CurrentCardXpBeforeGain = CurrentCardEnergyXP.GetCurrentValue();
		const float NewCardXp = CurrentCardEnergyXP.GetCurrentValue() + LocalIncomingCardEnergyXP;

		if (NewCardXp >= GetMaximumCardEnergyXP())
		{
			if (CurrentCardXpBeforeGain <= GetMaximumCardEnergyXP())
			{
				const float LeftoverCardXp = NewCardXp - GetMaximumCardEnergyXP();
				SetCurrentCardEnergy(GetCurrentCardEnergy() + 1);
				SetCurrentCardEnergyXP(LeftoverCardXp);
				if (GetCurrentCardEnergy() == GetMaximumCardEnergy())
				{
					SetCurrentCardEnergyXP(GetMaximumCardEnergyXP());
					return;
				}
			}
			else
			{
				SetCurrentCardEnergyXP(GetMaximumCardEnergyXP());
			}
		}
		else
		{
			if (NewCardXp < 0)
			{
				if (CurrentCardEnergy.GetCurrentValue() > 0)
				{
					SetCurrentCardEnergy(GetCurrentCardEnergy() - 1);
					SetCurrentCardEnergyXP(GetMaximumCardEnergyXP() - UKismetMathLibrary::Abs(NewCardXp));
				}
				else
				{
					SetCurrentCardEnergy(0);
					SetCurrentCardEnergyXP(0);
				}

			}
			else
			{
				SetCurrentCardEnergyXP(NewCardXp);
			}
		}
	}

	if (Data.EvaluatedData.Attribute == GetCurrentCardEnergyAttribute())
	{
		
		SetCurrentCardEnergy(FMath::Clamp(GetCurrentCardEnergy(), 0.0f, GetMaximumCardEnergy()));
		if (Data.EvaluatedData.Magnitude == GetMaximumCardEnergy())
		{
			SetCurrentCardEnergyXP(GetMaximumCardEnergy());
		}
	}

	if (Data.EvaluatedData.Attribute == GetCurrentCardEnergyXPAttribute())
	{
		SetCurrentCardEnergy(FMath::Clamp(GetCurrentCardEnergy(), 0.0f, GetMaximumCardEnergy()));
	}
}