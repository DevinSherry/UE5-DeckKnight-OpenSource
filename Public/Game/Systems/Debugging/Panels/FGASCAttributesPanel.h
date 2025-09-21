// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"
#include "FGASCAttributesPanel.generated.h"

USTRUCT()
struct FAttributeHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString AttributeName;

	UPROPERTY()
	float OldValue = 0.f;

	UPROPERTY()
	float NewValue = 0.f;

	UPROPERTY()
	FString InstigatorName;

	UPROPERTY()
	FString EffectName;

	UPROPERTY()
	FString ExecutionClassName;

	FAttributeHistoryEntry() {}
	FAttributeHistoryEntry(const FString& InAttr, float InOld, float InNew, const FString& InInstigator, const FString& InEffect, const FString& InExecClass)
		: AttributeName(InAttr), OldValue(InOld), NewValue(InNew), InstigatorName(InInstigator), EffectName(InEffect), ExecutionClassName(InExecClass){}
};

/**
 * @class FGASCAttributesPanel
 * @brief Represents a panel for managing attributes in the FGASC system.
 *
 * This class serves as the user interface component for displaying and editing
 * attributes in the FGASC (Flexible Grid Advanced Settings and Configuration) system.
 * It provides functionality to interact with the attribute data, such as viewing, updating,
 * and modifying attributes within the panel environment.
 *
 * The class manages the layout and behaviors of the visual components related
 * to attribute handling, ensuring a responsive and user-friendly experience.
 * It also integrates with underlying data structures or models to synchronize
 * the displayed information with the application's logic.
 *
 * Primary responsibilities include:
 * - Displaying a list of attributes with relevant details.
 * - Supporting user-triggered actions such as editing or removing attributes.
 * - Communicating attribute changes to the appropriate data processing layers.
 *
 * Ensure that this class is instantiated in an environment where it can properly
 * connect to the underlying data sources and event handling mechanisms for
 * optimal functionality.
 */
class GASCOURSE_API FGASCAttributesPanel : public IIGASCDebugPanel
{
public:
	FGASCAttributesPanel();
	~FGASCAttributesPanel();

	virtual const char* GetDebugPanelName() const override {return "Gameplay Attributes";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;

	static TMap<const FProperty*, TArray<FAttributeHistoryEntry>> AttributeHistories;

private:
	
	static TWeakObjectPtr<APawn> SelectedPawn;
	static TWeakObjectPtr<UAbilitySystemComponent> SelectedASC;
	static TArray<FOnAttributeChangeData> ChangedHistory;

	void InitializeAbilitySystemComponent(UAbilitySystemComponent* ASC);
};
