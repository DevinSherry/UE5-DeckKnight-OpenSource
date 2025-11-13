// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

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

private:
	
	static TWeakObjectPtr<APawn> SelectedPawn;
	static TWeakObjectPtr<UAbilitySystemComponent> SelectedASC;
	static TArray<FOnAttributeChangeData> ChangedHistory;

	TMap<FGameplayAttribute, bool> AttributeHistoryWindows;

	void InitializeAbilitySystemComponent(UAbilitySystemComponent* ASC);
};
