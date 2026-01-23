// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <imgui.h>

#include "Game/HUD/Cards/GASC_UI_CardHand.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * 
 */
class GASCOURSE_API FGASC_CardHandUILayoutDebug : public IIGASCDebugPanel
{
public:
	FGASC_CardHandUILayoutDebug();
	~FGASC_CardHandUILayoutDebug();
	
	virtual const char* GetDebugPanelName() const override {return "Card Hand Layout";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;
	
	UWorld* World = nullptr;
	UGASC_UI_CardHand* CardHand = nullptr;
	
private:
	
	static float CardSpacing;
	static float CardScale;
};
