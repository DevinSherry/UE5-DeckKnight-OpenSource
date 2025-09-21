// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * The FGASCDebugHub class serves as a central hub for managing and interacting with debug panels within
 * the GASCOURSE framework. It allows for the registration of debug panels, rendering of the hub, and controlling
 * visibility of the debug interface.
 */
class GASCOURSE_API FGASCDebugHub
{
public:
	FGASCDebugHub();
	~FGASCDebugHub();

	void RegisterDebugPanel(const TSharedPtr<IIGASCDebugPanel> &Panel);
	void DrawDebugHub();
	void UpdateCachedPawns(const TArray<TWeakObjectPtr<APawn>> &Pawns);

	void ShowDebugHub(const bool& bInOpen);
	bool IsDebugHubOpen() const; 

private:
	TMap<TSharedPtr<IIGASCDebugPanel>, bool> DebugPanels;
};
