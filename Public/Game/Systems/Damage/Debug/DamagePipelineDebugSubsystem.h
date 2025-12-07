// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "../Pipeline/GASC_DamagePipelineTypes.h"
#include "DamagePipelineDebugSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_DamagePipelineDebugSubsystem, Log, All);

/**
 *
 */
UCLASS()
class GASCOURSE_API UDamagePipelineDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION()
	void LogDamageEvent(const FDamageLogEntry& DamageContext);
	
	UFUNCTION()
	TArray<FDamageLogEntry> GetDamageLogEntries();

	UFUNCTION()
	TArray<FDamageLogEntry> GetDamageLogEntriesForActorID(uint32 InActorID);
	
	UFUNCTION()
	FDamageLogEntry GetDamageLogEntryByDamageID(uint32 InDamageID);
	
	UFUNCTION()
	AActor* GetActorFromID(const uint32& InActorID);
	
	UFUNCTION()
	void SimulateDamageFromID(uint32 DamageID);
	
	uint32 GenerateDebugDamageUniqueID();
	
private:
	
	UPROPERTY()
	TArray<FDamageLogEntry> DamageLogEntries;
	
	UPROPERTY()
	TMap<FGameplayAttribute, float> InstigatorAttributesBackup;
	
	void TempApplyAttributeModToInstigator(FDamageLogEntry DamageLogEntry, AActor* InInstigator);
	void RestoreBackupAttributesToInstigator(AActor* InInstigator);
	
};
