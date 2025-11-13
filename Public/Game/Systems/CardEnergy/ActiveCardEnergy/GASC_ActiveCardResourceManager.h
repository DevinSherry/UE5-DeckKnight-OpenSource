// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActiveCardResourceSettings.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GASC_ActiveCardResourceManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_ActiveCardResourceSubsystem, Log, All);

/**
 * @class FActiveCardEnergyXPHistoryEntry
 * @brief Represents a historical record of energy and experience point (XP) changes for an active card.
 *
 * The FActiveCardEnergyXPHistoryEntry class is designed to encapsulate the state and data associated with
 * changes in energy or XP for an active card over time. It is used to track and log updates within the
 * card's lifecycle, providing an audit trail for energy and XP modifications.
 *
 * This class is particularly useful for debugging, analytics, or gameplay systems that require detailed
 * insights into how an active card's energy and XP values have evolved in specific scenarios.
 *
 * It stores relevant details of each entry, enabling systems to reconstruct or analyze historical updates
 * when needed. Each instance represents a single update entry within the broader history of an active card.
 */
USTRUCT()
struct FActiveCardEnergyXPHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag ActiveCardEnergyXPEventTag;

	UPROPERTY()
	float ActiveCardEnergyXPBaseValue = 0.f;

	UPROPERTY()
	float ActiveCardEnergyXPModifiedValue = 0.f;

	UPROPERTY()
	FString InstigatorName;

	UPROPERTY()
	FString ModificationToolTip;

	FActiveCardEnergyXPHistoryEntry() {}
	FActiveCardEnergyXPHistoryEntry(FGameplayTag InActiveCardEnergyXPEventTag, float InActiveCardEnergyXPBaseValue, float InActiveCardEnergyXPModifiedValue, const FString& InInstigator, const FString& InToolTip)
		: ActiveCardEnergyXPEventTag(InActiveCardEnergyXPEventTag), ActiveCardEnergyXPBaseValue(InActiveCardEnergyXPBaseValue), ActiveCardEnergyXPModifiedValue(InActiveCardEnergyXPModifiedValue), InstigatorName(InInstigator),
	ModificationToolTip(InToolTip){}
};

/**
 * @class UGASC_ActiveCardResourceManager
 * @brief Manages active card resources within the application.
 *
 * The UGASC_ActiveCardResourceManager class encapsulates the functionality for handling
 * card resources currently in use. This includes managing resource allocation, ensuring
 * proper state handling, and releasing resources when they are no longer needed.
 *
 * It provides methods to support efficient interaction with active card resources,
 * helping maintain consistency and prevent conflicts during their lifecycle.
 *
 * This class is especially useful in scenarios that require dynamic resource management
 * for cards, supporting functionality such as updating card states, accessing specific
 * resources, or monitoring the lifecycle of the resources.
 *
 * Thread-safety and performance considerations may be implemented depending on specific
 * application requirements.
 */
UCLASS()
class GASCOURSE_API UGASC_ActiveCardResourceManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// Called when subsystem is created for a LocalPlayer
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Called when subsystem is destroyed
	virtual void Deinitialize() override;
	
public:

	UFUNCTION(BlueprintCallable, Category = "GASCourse|ActiveCardResourceManager")
	bool RegisterPlayer(APlayerController* InLocalPlayer);

	UFUNCTION(BlueprintCallable, Category = "GASCourse|ActiveCardResourceManager")
	bool UnRegisterPlayer(APlayerController* InLocalPlayer);

	void GameplayEventCallback(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	void LoadActiveCardResourceMapping();

	UFUNCTION()
	FORCEINLINE TArray<FActiveCardEnergyXPHistoryEntry>& GetActiveCardEnergyHistory()
	{
		return ActiveCardEnergyXPHistory;
	};

private:

	UPROPERTY()
	APlayerController* RegisteredLocalPlayerController = nullptr;

	UPROPERTY()
	UAbilitySystemComponent* RegisteredLocalPlayerASC = nullptr;

	FDelegateHandle MyHandle;

	UPROPERTY()
	const UActiveCardResourceSettings* ActiveCardResourceSettings = nullptr;

	UPROPERTY()
	const UActiveCardResourceEventMappingData* ActiveCardResourceEventMappingData = nullptr;

	UPROPERTY()
	TArray<FActiveCardEnergyXPHistoryEntry> ActiveCardEnergyXPHistory;
};
