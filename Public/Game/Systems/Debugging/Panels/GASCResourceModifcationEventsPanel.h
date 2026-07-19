// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <imgui.h>

#include "Game/DeveloperSettings/UGASC_AbilitySystemSettings.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineTypes.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * @class FGASCResourceModifcationEventsPanel
 * @brief A debug panel implementation that provides debugging and visualization tools for the Damage Events Pipeline System.
 *
 * This class is derived from the IIGASCDebugPanel interface and is part of the GASCourse debug framework.
 * It is primarily responsible for managing and displaying information related to the Damage Events Pipeline,
 * such as interactions with pawns and their associated damage event pipelines.
 */

class GASCOURSE_API FGASCResourceModifcationEventsPanel : public IIGASCDebugPanel
{
public:
	
	FGASCResourceModifcationEventsPanel();
	~FGASCResourceModifcationEventsPanel();
	
	virtual const char* GetDebugPanelName() const override {return "Resource Modification Pipeline System";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;
	
	const UGASC_AbilitySystemSettings* AbilitySystemSettings = nullptr;
	
private:
	static TWeakObjectPtr<APawn> SelectedPawn;
	static EGASC_ResourcePipelineType SelectedPipelineType;
	
	static TMap<uint32, bool> DrawHitResultStates;
	static TMap<uint32, ImVec4> DrawHitResultColors;
	static TMap<uint32, uint32> DrawHitResultRadii;
	
	static bool bShowOnlyAsInstigator;
	static bool bFilterOutSimulated;
	static int DamageModEventType;
	static int DamageType;
	static int DamageModContextFilter;
	
	static bool bImmuneDamageAll;
	static bool bImmuneDamageFire;
	
};
