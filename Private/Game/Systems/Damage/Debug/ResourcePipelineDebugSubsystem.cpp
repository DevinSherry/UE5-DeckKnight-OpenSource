// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Damage/Debug/ResourcePipelineDebugSubsystem.h"
#include "EngineUtils.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineSubsystem.h"
#include "GASCourse/GASCourseCharacter.h"

void UResourcePipelineDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AbilitySystemSettings = GetDefault<UGASC_AbilitySystemSettings>();
}

void UResourcePipelineDebugSubsystem::LogResourceModEvent(const FResourceLogEntry& ResourceModContext)
{
	ResourceLogEntries.EmplaceAt(0, ResourceModContext);
}

TArray<FResourceLogEntry> UResourcePipelineDebugSubsystem::GetResourceModLogEntries()
{
	return ResourceLogEntries;
}

TArray<FResourceLogEntry> UResourcePipelineDebugSubsystem::GetResourceLogEntriesForActorID(uint32 InActorID)
{
	TArray<FResourceLogEntry> FilteredLogEntries;
	FilteredLogEntries.Reserve(ResourceLogEntries.Num());

	for (const FResourceLogEntry& Entry : ResourceLogEntries)
	{
		if (GetTypeHash(Entry.ResourceTargetID) == InActorID || GetTypeHash(Entry.ResourceInstigatorID) == InActorID)
		{
			FilteredLogEntries.Add(Entry);
		}
	}
	return FilteredLogEntries;
}

FResourceLogEntry UResourcePipelineDebugSubsystem::GetResourceLogEntryByResourceID(uint32 InResourceID)
{
	FResourceLogEntry LastResourceLogEntry = FResourceLogEntry();
	for (const FResourceLogEntry& Entry : ResourceLogEntries)
	{
		if (GetTypeHash(Entry.ResourceID) == InResourceID)
		{
			LastResourceLogEntry = Entry;
			break;
		}
	}
	return LastResourceLogEntry;
}

void UResourcePipelineDebugSubsystem::SimulateResourceModFromID(uint32 ResourceID)
{
	FResourceLogEntry ResourceLogEntry = GetResourceLogEntryByResourceID(ResourceID);
	if (ResourceLogEntry.ResourceID)
	{
		FResourceModificationContext ResourceModContext;
		if (GetActorFromID(ResourceLogEntry.ResourceTargetID))
		{
			ResourceModContext.HitContext.HitTarget = GetActorFromID(ResourceLogEntry.ResourceTargetID);
			ResourceModContext.HitContext.HitTargetTagsContainer = &ResourceLogEntry.HitTargetTagsContainer;
		}
		if (GetActorFromID(ResourceLogEntry.ResourceInstigatorID))
		{
			ResourceModContext.HitContext.HitInstigator = GetActorFromID(ResourceLogEntry.ResourceInstigatorID);
			ResourceModContext.HitContext.HitInstigatorTagsContainer = &ResourceLogEntry.HitInstigatorTagsContainer;
		}
		
		ResourceModContext.HitContext.HitTimeStamp = ResourceLogEntry.ResourceTimeStamp;
		ResourceModContext.HitContext.HitContextTagsContainer = &ResourceLogEntry.HitContextTagsContainer;
		
		FDamagePipelineContext DamagePipelineContext;
		for (FGameplayTag Tag : ResourceLogEntry.HitContextTagsContainer)
		{
			if (Tag.MatchesTag(DamageType_Root))
			{
				DamagePipelineContext.DamageType = Tag;
				break;
			}
		}
		DamagePipelineContext.GrantedTags = ResourceLogEntry.HitContextTagsContainer;
		DamagePipelineContext.GrantedTags.AddTagFast(Data_DebugSimulated);
		
		if (UGASC_ResourcePipelineSubsystem* DamagePipelineSubsystem = GetWorld()->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
		{
			TempApplyAttributeModToInstigator(ResourceLogEntry, ResourceModContext.HitContext.HitInstigator.Get());
			if (ResourceLogEntry.bIsDamageEffect)
			{
				DamagePipelineSubsystem->ApplyDamageToTarget(ResourceModContext.HitContext.HitTarget, ResourceModContext.HitContext.HitInstigator, ResourceLogEntry.BaseResourceValue, DamagePipelineContext);
			}
			else
			{
				DamagePipelineSubsystem->ApplyHealToTarget(ResourceModContext.HitContext.HitTarget, ResourceModContext.HitContext.HitInstigator, ResourceLogEntry.BaseResourceValue, DamagePipelineContext);
			}
			RestoreBackupAttributesToInstigator(ResourceModContext.HitContext.HitInstigator.Get());
		}
	}
}

AActor* UResourcePipelineDebugSubsystem::GetActorFromID(const uint32& InActorID)
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

uint32 UResourcePipelineDebugSubsystem::GenerateDebugResourceModUniqueID()
{
	static std::atomic<uint32> ResourceIDCounter {1};
	return ResourceIDCounter.fetch_add(1, std::memory_order_relaxed);
}

void UResourcePipelineDebugSubsystem::TempApplyAttributeModToInstigator(FResourceLogEntry ResourceLogEntry, AActor* InInstigator)
{
	TArray<FGameplayAttribute> IgnoredSimulatedAttributes;
	if (AbilitySystemSettings)
	{
		IgnoredSimulatedAttributes = AbilitySystemSettings->SimulatedDamageIgnoreAttributes;
	}
	
	if (AGASCourseCharacter* InstigatorPawn = Cast<AGASCourseCharacter>(InInstigator))
	{
		if (UAbilitySystemComponent* InstigatorASC = InstigatorPawn->GetAbilitySystemComponent())
		{
			TArray<FGameplayAttribute> AllAttributes;
			InstigatorASC->GetAllAttributes(AllAttributes);
			
			TArray<FString> AttributeNames;
			ResourceLogEntry.Attributes.GetKeys(AttributeNames);
			
			InstigatorAttributesBackup.Empty();
			
			for (const FGameplayAttribute& Attribute : AllAttributes)
			{
				if (AttributeNames.Contains(Attribute.AttributeName) && !IgnoredSimulatedAttributes.Contains(Attribute))
				{
					float Original = InstigatorASC->GetNumericAttribute(Attribute);
					float NewValue = ResourceLogEntry.Attributes.FindChecked(Attribute.GetName());
					
					InstigatorAttributesBackup.Add(Attribute, Original);
					InstigatorASC->ApplyModToAttribute(Attribute, EGameplayModOp::Override, NewValue);
				}
			}
		}
	}
}

void UResourcePipelineDebugSubsystem::RestoreBackupAttributesToInstigator(AActor* InInstigator)
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
