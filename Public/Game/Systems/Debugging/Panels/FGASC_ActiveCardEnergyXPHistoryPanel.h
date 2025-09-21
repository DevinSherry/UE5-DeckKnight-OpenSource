// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Systems/CardEnergy/ActiveCardEnergy/GASC_ActiveCardResourceManager.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * @class FGASC_ActiveCardEnergyXPHistoryPanel
 * @brief Represents a debug panel for displaying the history of active card energy and XP events.
 *
 * This class is responsible for implementing an in-game debugging panel that shows detailed
 * information about resource-related events, specifically for active card energy and XP tracking.
 * It provides functionality to display and update the panel contents based on the cached pawns.
 *
 * This class implements the IIGASCDebugPanel interface and overrides its methods to define
 * panel behavior and update mechanisms.
 */
class GASCOURSE_API FGASC_ActiveCardEnergyXPHistoryPanel : public IIGASCDebugPanel
{
public:

	FGASC_ActiveCardEnergyXPHistoryPanel();
	~FGASC_ActiveCardEnergyXPHistoryPanel();

	virtual const char* GetDebugPanelName() const override {return "Active Card Resource Events Panel";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;

private:
	
	static TWeakObjectPtr<APawn> SelectedPawn;
	static TWeakObjectPtr<UGASC_ActiveCardResourceManager> SelectedResourceManager;
	static TArray<FActiveCardEnergyXPHistoryEntry> ActiveCardEnergyXPHistoryEntries;
};
