// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AttributeSet.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Pipeline/GASC_DamagePipelineTypes.h"
#include "DamagePipelineDebugSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_DamagePipelineDebugSubsystem, Log, All);

/**
 * @class UDamagePipelineDebugSubsystem
 * @brief A subsystem responsible for managing and debugging the damage pipeline in the game.
 *
 * The UDamagePipelineDebugSubsystem provides functionality to track, visualize, and debug
 * the behavior of the damage pipeline, assisting developers in understanding and validating
 * the flow of damage events within the game. It integrates with the game's subsystem architecture
 * and offers tools to analyze damage calculations and processes.
 *
 * Key capabilities of the subsystem may include:
 * - Monitoring the flow of damage events through different components of the pipeline.
 * - Logging detailed debug information for damage events.
 * - Visualizing damage impact in various stages.
 * - Providing insights into modifier applications and cumulative damage calculations.
 *
 * This subsystem should typically be utilized during debugging or development phases
 * and can be disabled or configured for performance in production environments.
 *
 * Note: Specific methods and functionality provided by this class will depend on the implementation.
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
