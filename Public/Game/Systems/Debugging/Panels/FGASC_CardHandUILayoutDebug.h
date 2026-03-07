// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/HUD/Cards/GASC_UI_CardHand.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

/**
 * @class FGASC_CardHandUILayoutDebug
 *
 * @brief A debug utility class for managing and debugging the layout of a card hand in a graphical user interface.
 *
 * This class provides tools and utilities designed to assist in debugging the layout of card hands
 * within a custom user interface. It enables visualization of layout adjustments and ensures that
 * the cards are positioned correctly during development.
 *
 * The FGASC_CardHandUILayoutDebug class can be used to inspect the spacing, alignment, and overall
 * arrangement of cards to help detect any layout-related issues in the development environment.
 *
 * Use this class to streamline debugging of hand card layouts when developing card-based UI systems.
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
	
	static float MinimumCardSpacing;
	static float MaximumCardSpacing;
	static float CardScale;
	static float AnimationSpeed;
	static float SelectedShowYAmount;
	static float SelectedCardScale;
	static float SelectedShowXAmount;
	static float HandWidthPercentage;
	static float CardArcAmount;
	static float ExtraYOffset;
	static float CardAngle;
	
	static bool bKeepStackOnLeft;
	static bool bDoStackCardsOnSides;
	static bool bSelectNextCardOnUse;
};
