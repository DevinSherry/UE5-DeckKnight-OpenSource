// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/CardEnergy/ActiveCardEnergy/GASC_ActiveCardResourceManager.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"
#include "Game/Character/Player/GASCoursePlayerState.h"
#include "Game/DeveloperSettings/UGASC_AbilitySystemSettings.h"
#include "Game/GameplayAbilitySystem/GASCourseGameplayEffect.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/Systems/CardEnergy/GASCourseCardEnergyExecution.h"

DEFINE_LOG_CATEGORY(LOG_GASC_ActiveCardResourceSubsystem);

//TODO: Make ImGui debugger that displays event information from this subsystem!

void UGASC_ActiveCardResourceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveCardResourceSettings = GetDefault<UActiveCardResourceSettings>();
	if (!ActiveCardResourceSettings)
	{
		UE_LOG(LOG_GASC_ActiveCardResourceSubsystem, Warning, TEXT("ActiveCardResourceSettings not found!"));
	}

	FSoftObjectPath AssetPath;
	const TSoftObjectPtr<UActiveCardResourceEventMappingData>& SoftPtr = ActiveCardResourceSettings->ActiveCardResourceEventMappingData;
	if (!SoftPtr.IsValid())
	{
		AssetPath = SoftPtr.ToSoftObjectPath();
	}
	else
	{
		 ActiveCardResourceEventMappingData = SoftPtr.Get();
	}

	// Bulk load all objects at once
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(
	AssetPath,
	FStreamableDelegate::CreateUObject(this, &UGASC_ActiveCardResourceManager::LoadActiveCardResourceMapping)
	);
}

void UGASC_ActiveCardResourceManager::Deinitialize()
{
	Super::Deinitialize();
	if (ActiveCardResourceSettings)
	{
		ActiveCardResourceSettings = nullptr;
	}
}

bool UGASC_ActiveCardResourceManager::RegisterPlayer(APlayerController* InLocalPlayer)
{
	if (InLocalPlayer && !RegisteredLocalPlayerController)
	{
		RegisteredLocalPlayerController = InLocalPlayer;
		if (AGASCoursePlayerState* PS = Cast<AGASCoursePlayerState>(RegisteredLocalPlayerController->PlayerState))
		{
			RegisteredLocalPlayerASC = PS->GetAbilitySystemComponent();
			if (RegisteredLocalPlayerASC)
			{
				MyHandle = RegisteredLocalPlayerASC->AddGameplayEventTagContainerDelegate(FGameplayTagContainer(Event_Gameplay), FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGASC_ActiveCardResourceManager::GameplayEventCallback));
				return true;
			}
		}
	}
	return false;
}

bool UGASC_ActiveCardResourceManager::UnRegisterPlayer(APlayerController* InLocalPlayer)
{
	if (InLocalPlayer == RegisteredLocalPlayerController)
	{
		RegisteredLocalPlayerController = nullptr;
		RegisteredLocalPlayerASC = nullptr;
		MyHandle.Reset();
		ActiveCardEnergyXPHistory.Empty();
		return true;
	}
	return false;
}

void UGASC_ActiveCardResourceManager::GameplayEventCallback(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	if (ActiveCardResourceEventMappingData)
	{
		bool bMappingFound = false;
		float MappingValue = ActiveCardResourceEventMappingData->GetGameplayEventMapping(MatchingTag, bMappingFound, 1.0f);
		if (bMappingFound)
		{
			UE_LOG(LOG_GASC_ActiveCardResourceSubsystem, Log, TEXT("Event Tag: %s | Instigator: %s | Target: %s | Mapping Value = %f"),
				*MatchingTag.ToString(), *Payload->Instigator.GetFullName(), *Payload->Target.GetFullName(), MappingValue);
			
			const AActor* ConstActor = Payload->Instigator.Get();
			AActor* Actor = const_cast<AActor*>(ConstActor);

			const AActor* ConstTarget = Payload->Target.Get();
			AActor* Target = const_cast<AActor*>(ConstTarget);
			
			if (AGASCoursePlayerCharacter* SourceActor = Cast<AGASCoursePlayerCharacter>(Actor))
			{
				if (UAbilitySystemComponent* SourceASC = SourceActor->GetAbilitySystemComponent())
				{
					TSubclassOf<UGameplayEffectExecutionCalculation> IncomingActiveCardEnergyXPCalculationClass;
					if (const UGASC_AbilitySystemSettings* AbilitySystemSettings = GetDefault<UGASC_AbilitySystemSettings>())
					{
						IncomingActiveCardEnergyXPCalculationClass = AbilitySystemSettings->CardResourceExecution;
						if (!IncomingActiveCardEnergyXPCalculationClass)
						{
							UE_LOG(LogTemp, Warning, TEXT("Health Calculation is not valid!"));
							return;
						}
					}

					FActiveCardEnergyXPHistoryEntry NewEntry;
					NewEntry.ActiveCardEnergyXPEventTag = MatchingTag;
					NewEntry.ActiveCardEnergyXPBaseValue = MappingValue;
					NewEntry.InstigatorName = Target->GetFullName();
					ActiveCardEnergyXPHistory.Add(NewEntry);

					UGASCourseGameplayEffect* IncomingActiveCardEnergyXPEffect = NewObject<UGASCourseGameplayEffect>(GetTransientPackage());
					IncomingActiveCardEnergyXPEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
					if(IncomingActiveCardEnergyXPEffect)
					{
						FGameplayEffectExecutionDefinition ActiveCardEnergyXPExecutionDefinition;
						ActiveCardEnergyXPExecutionDefinition.CalculationClass = IncomingActiveCardEnergyXPCalculationClass;
						if(ActiveCardEnergyXPExecutionDefinition.CalculationClass)
						{
							FGameplayTagContainer DynamicGrantedTags;
							DynamicGrantedTags.AddTag(Data_ActiveCardEnergyXP);
							DynamicGrantedTags.AddTag(MatchingTag);
							IncomingActiveCardEnergyXPEffect->Executions.Emplace(ActiveCardEnergyXPExecutionDefinition);
							FGameplayEffectContext* ContextHandle = UAbilitySystemGlobals::Get().AllocGameplayEffectContext();
							ContextHandle->AddInstigator(SourceActor, SourceActor);
							const FGameplayEffectSpecHandle IncomingActiveCardEnergyXPEffectHandle = FGameplayEffectSpecHandle(new FGameplayEffectSpec(IncomingActiveCardEnergyXPEffect, FGameplayEffectContextHandle(ContextHandle), 1.0f));
							UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(IncomingActiveCardEnergyXPEffectHandle, Data_IncomingCardEnergyXP, MappingValue);
							UAbilitySystemBlueprintLibrary::AddGrantedTags(IncomingActiveCardEnergyXPEffectHandle, DynamicGrantedTags);
							SourceASC->ApplyGameplayEffectSpecToTarget(*IncomingActiveCardEnergyXPEffectHandle.Data.Get(), SourceASC);
						}
					}
				}
			}
		}
	}
}

void UGASC_ActiveCardResourceManager::LoadActiveCardResourceMapping()
{
	const TSoftObjectPtr<UActiveCardResourceEventMappingData>& SoftPtr = ActiveCardResourceSettings->ActiveCardResourceEventMappingData;
	ActiveCardResourceEventMappingData = SoftPtr.Get();
	UE_LOG(LOG_GASC_ActiveCardResourceSubsystem, Log, TEXT("ActiveCardResourceEventMappingData available!"));
}
