// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Systems/Damage/Pipeline/GASC_DamagePipelineTypes.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * 
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
};
