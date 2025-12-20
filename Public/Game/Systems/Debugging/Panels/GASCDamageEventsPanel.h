// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <imgui.h>

#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineTypes.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * @class FGASCDamageEventsPanel
 * @brief A debug panel implementation that provides debugging and visualization tools for the Damage Events Pipeline System.
 *
 * This class is derived from the IIGASCDebugPanel interface and is part of the GASCourse debug framework.
 * It is primarily responsible for managing and displaying information related to the Damage Events Pipeline,
 * such as interactions with pawns and their associated damage event pipelines.
 */

class GASCOURSE_API FGASCDamageEventsPanel : public IIGASCDebugPanel
{
public:
	
	FGASCDamageEventsPanel();
	~FGASCDamageEventsPanel();
	
	virtual const char* GetDebugPanelName() const override {return "Damage Events Pipeline System";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;
	
private:
	static TWeakObjectPtr<APawn> SelectedPawn;
	static EGASC_DamagePipelineType SelectedPipelineType;
	
	static TMap<uint32, bool> DrawHitResultStates;
	static TMap<uint32, ImVec4> DrawHitResultColors;
	static TMap<uint32, uint32> DrawHitResultRadii;
	
	static bool bShowOnlyAsInstigator;
	static bool bFilterOutSimulated;
	static int DamageModEventType;
	static int DamageType;
	static int DamageModContextFilter;
	
};
