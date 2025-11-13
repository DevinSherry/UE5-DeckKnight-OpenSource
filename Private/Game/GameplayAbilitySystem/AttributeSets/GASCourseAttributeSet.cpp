// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectExecutionCalculation.h" 
#include "Net/UnrealNetwork.h"

UGASCourseAttributeSet::UGASCourseAttributeSet()
{
	AbilitySystemSettings = GetDefault<UGASC_AbilitySystemSettings>();
}

void UGASCourseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UGASCourseAttributeSet, OneAttribute, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGASCourseAttributeSet, TwoAttribute, COND_None, REPNOTIFY_Always);
}

void UGASCourseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (UWorld* World = ASC->GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				if (UGASC_DebugSubsystem* Subsystem = GI->GetSubsystem<UGASC_DebugSubsystem>())
				{
					FAttributeHistoryEntry Entry;
					Entry.AttributeName = Data.EvaluatedData.Attribute.AttributeName;
					Entry.NewValue = ASC->GetNumericAttribute(Data.EvaluatedData.Attribute);
					Entry.OldValue = Entry.NewValue - Data.EvaluatedData.Magnitude;
					Entry.InstigatorName = Data.EffectSpec.GetContext().GetOriginalInstigator() ? Data.EffectSpec.GetContext().GetOriginalInstigator()->GetName()
						: "None";
					Entry.EffectName = Data.EffectSpec.Def ? Data.EffectSpec.Def.GetFullName(): "None";
					if (Data.EffectSpec.Def->Executions.Num() > 0)
					{
						Entry.ExecutionClassName = Data.EffectSpec.Def->Executions[0].CalculationClass->GetFullName();
					}
					else
					{
						Entry.ExecutionClassName = "None";
					}
					Subsystem->AttributeHistory.FindOrAdd(ASC->GetAvatarActor()).Add(Entry);
				}
			}
		}
	}
}

void UGASCourseAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute,
                                                         const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void UGASCourseAttributeSet::OnRep_OneAttribute(const FGameplayAttributeData& OldOneAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGASCourseAttributeSet, OneAttribute, OldOneAttribute);
}

void UGASCourseAttributeSet::OnRep_TwoAttribute(const FGameplayAttributeData& OldTwoAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGASCourseAttributeSet, TwoAttribute, OldTwoAttribute);
}
