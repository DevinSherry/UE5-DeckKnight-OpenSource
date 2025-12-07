// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/Debug/DamagePipelineDebugSubsystem.h"
#include "EngineUtils.h"
#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineSubsystem.h"
#include "GASCourse/GASCourseCharacter.h"

void UDamagePipelineDebugSubsystem::LogDamageEvent(const FDamageLogEntry& DamageContext)
{
	//DamageLogEntries.Add(DamageContext);
	DamageLogEntries.EmplaceAt(0, DamageContext);
}

TArray<FDamageLogEntry> UDamagePipelineDebugSubsystem::GetDamageLogEntries()
{
	return DamageLogEntries;
}

TArray<FDamageLogEntry> UDamagePipelineDebugSubsystem::GetDamageLogEntriesForActorID(uint32 InActorID)
{
	TArray<FDamageLogEntry> FilteredLogEntries;
	FilteredLogEntries.Reserve(DamageLogEntries.Num());

	for (const FDamageLogEntry& Entry : DamageLogEntries)
	{
		if (GetTypeHash(Entry.DamageTargetID) == InActorID || GetTypeHash(Entry.DamageInstigatorID) == InActorID)
		{
			FilteredLogEntries.Add(Entry);
		}
	}
	return FilteredLogEntries;
}

FDamageLogEntry UDamagePipelineDebugSubsystem::GetDamageLogEntryByDamageID(uint32 InDamageID)
{
	FDamageLogEntry LastDamageLogEntry = FDamageLogEntry();
	for (const FDamageLogEntry& Entry : DamageLogEntries)
	{
		if (GetTypeHash(Entry.DamageID) == InDamageID)
		{
			LastDamageLogEntry = Entry;
			break;
		}
	}
	return LastDamageLogEntry;
}

void UDamagePipelineDebugSubsystem::SimulateDamageFromID(uint32 DamageID)
{
	FDamageLogEntry DamageLogEntry = GetDamageLogEntryByDamageID(DamageID);
	if (DamageLogEntry.DamageID)
	{
		FDamageModificationContext DamageContext;
		if (GetActorFromID(DamageLogEntry.DamageTargetID))
		{
			DamageContext.HitContext.HitTarget = GetActorFromID(DamageLogEntry.DamageTargetID);
			DamageContext.HitContext.HitTargetTagsContainer = &DamageLogEntry.HitTargetTagsContainer;
		}
		if (GetActorFromID(DamageLogEntry.DamageInstigatorID))
		{
			DamageContext.HitContext.HitInstigator = GetActorFromID(DamageLogEntry.DamageInstigatorID);
			DamageContext.HitContext.HitInstigatorTagsContainer = &DamageLogEntry.HitInstigatorTagsContainer;
		}
		
		DamageContext.HitContext.HitTimeStamp = DamageLogEntry.DamageTimeStamp;
		DamageContext.HitContext.HitContextTagsContainer = &DamageLogEntry.HitContextTagsContainer;
		
		FDamagePipelineContext DamagePipelineContext;
		for (FGameplayTag Tag : DamageLogEntry.HitContextTagsContainer)
		{
			if (Tag.MatchesTag(DamageType_Root))
			{
				DamagePipelineContext.DamageType = Tag;
				break;
			}
		}
		DamagePipelineContext.GrantedTags = DamageLogEntry.HitContextTagsContainer;
		DamagePipelineContext.GrantedTags.AddTagFast(DamageType_DebugSimulated);
		
		if (UGASC_DamagePipelineSubsystem* DamagePipelineSubsystem = GetWorld()->GetSubsystem<UGASC_DamagePipelineSubsystem>())
		{
			TempApplyAttributeModToInstigator(DamageLogEntry, DamageContext.HitContext.HitInstigator.Get());
			if (DamageLogEntry.bIsDamageEffect)
			{
				DamagePipelineSubsystem->ApplyDamageToTarget(DamageContext.HitContext.HitTarget, DamageContext.HitContext.HitInstigator, DamageLogEntry.BaseDamageValue, DamagePipelineContext);
			}
			else
			{
				DamagePipelineSubsystem->ApplyHealToTarget(DamageContext.HitContext.HitTarget, DamageContext.HitContext.HitInstigator, DamageLogEntry.BaseDamageValue, DamagePipelineContext);
			}
			RestoreBackupAttributesToInstigator(DamageContext.HitContext.HitInstigator.Get());
		}
	}
}

AActor* UDamagePipelineDebugSubsystem::GetActorFromID(const uint32& InActorID)
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetUniqueID() == InActorID)
		{
			return Actor;
		}
	}
	return nullptr;
}

uint32 UDamagePipelineDebugSubsystem::GenerateDebugDamageUniqueID()
{
	static std::atomic<uint32> DamageIDCounter {1};
	return DamageIDCounter.fetch_add(1, std::memory_order_relaxed);
}

void UDamagePipelineDebugSubsystem::TempApplyAttributeModToInstigator(FDamageLogEntry DamageLogEntry, AActor* InInstigator)
{
	if (AGASCourseCharacter* InstigatorPawn = Cast<AGASCourseCharacter>(InInstigator))
	{
		if (UAbilitySystemComponent* InstigatorASC = InstigatorPawn->GetAbilitySystemComponent())
		{
			TArray<FGameplayAttribute> AllAttributes;
			InstigatorASC->GetAllAttributes(AllAttributes);
			
			TArray<FString> AttributeNames;
			DamageLogEntry.Attributes.GetKeys(AttributeNames);
			
			InstigatorAttributesBackup.Empty();
			
			for (const FGameplayAttribute& Attribute : AllAttributes)
			{
				if (AttributeNames.Contains(Attribute.AttributeName))
				{
					float Original = InstigatorASC->GetNumericAttribute(Attribute);
					float NewValue = DamageLogEntry.Attributes.FindChecked(Attribute.GetName());
					
					InstigatorAttributesBackup.Add(Attribute, Original);
					InstigatorASC->ApplyModToAttribute(Attribute, EGameplayModOp::Override, NewValue);
				}
			}
		}
	}
}

void UDamagePipelineDebugSubsystem::RestoreBackupAttributesToInstigator(AActor* InInstigator)
{
	if (AGASCourseCharacter* InstigatorPawn = Cast<AGASCourseCharacter>(InInstigator))
	{
		if (UAbilitySystemComponent* InstigatorASC = InstigatorPawn->GetAbilitySystemComponent())
		{
			TArray<FGameplayAttribute> Attributes;
			InstigatorAttributesBackup.GetKeys(Attributes);
			for (const auto& Pair : InstigatorAttributesBackup)
			{
				const FGameplayAttribute& Attribute = Pair.Key;
				float OriginalValue = Pair.Value;

				InstigatorASC->ApplyModToAttribute(Attribute, EGameplayModOp::Override, OriginalValue);
			}
		}
	}
	InstigatorAttributesBackup.Empty();
}
