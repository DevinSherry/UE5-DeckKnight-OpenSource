// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AttributeSet.h"
#include "Subsystems/WorldSubsystem.h"
#include "Game/DeveloperSettings/UGASC_AbilitySystemSettings.h"
#include "ResourcePipelineDebugSubsystem.generated.h"

struct FResourceLogEntry;
class UGASC_ResourcePipelineTypes;

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_DamagePipelineDebugSubsystem, Log, All);

/**
 * @class UResourcePipelineDebugSubsystem
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
class GASCOURSE_API UResourcePipelineDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION()
	void LogResourceModEvent(const FResourceLogEntry& ResourceModContext);
	
	UFUNCTION()
	TArray<FResourceLogEntry> GetResourceModLogEntries();

	UFUNCTION()
	TArray<FResourceLogEntry> GetResourceLogEntriesForActorID(uint32 InActorID);
	
	UFUNCTION()
	FResourceLogEntry GetResourceLogEntryByResourceID(uint32 InResourceID);
	
	UFUNCTION()
	AActor* GetActorFromID(const uint32& InActorID);
	
	UFUNCTION()
	void SimulateResourceModFromID(uint32 ResourceID);
	
	uint32 GenerateDebugResourceModUniqueID();
	
	UPROPERTY()
	const UGASC_AbilitySystemSettings* AbilitySystemSettings = nullptr;
	
private:
	
	UPROPERTY()
	TArray<FResourceLogEntry> ResourceLogEntries;
	
	UPROPERTY()
	TMap<FGameplayAttribute, float> InstigatorAttributesBackup;
	
	void TempApplyAttributeModToInstigator(FResourceLogEntry ResourceLogEntry, AActor* InInstigator);
	void RestoreBackupAttributesToInstigator(AActor* InInstigator);
	
};
