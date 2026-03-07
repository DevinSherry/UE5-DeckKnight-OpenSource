// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ImGuiTextureHandle.h"
#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"
#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"

/**
 * @class FGASC_PlayerDeckManagerPanel
 * @brief Handles the management and UI representation of the player's card deck.
 *
 * This class is responsible for providing functionality to manage and interact
 * with the player's deck of cards in the game. It facilitates adding, removing,
 * and organizing cards in the player's deck, as well as updating the user interface
 * to reflect changes in real-time.
 *
 * Responsibilities of this class include:
 * - Displaying the current state of the player's deck.
 * - Allowing users to perform deck customization operations.
 * - Communicating changes to the deck to relevant game systems.
 *
 * Use this class as a part of the Deck Management module, integrating it with
 * other gameplay systems and UI components.
 *
 * Design Notes:
 * - This class assumes a standard card object structure, adhering to the game's
 *   card management conventions.
 * - Events and bindings should be implemented externally to allow this class
 *   to interact with other game subsystems.
 *
 * Thread Safety:
 * - It is assumed that this class operates on the game's main thread and that
 *   no multithreading is utilized in managing UI or deck operations.
 */

class GASCOURSE_API FGASC_PlayerDeckManagerPanel : public IIGASCDebugPanel
{
	
public:
	FGASC_PlayerDeckManagerPanel();
	~FGASC_PlayerDeckManagerPanel();
	
	virtual const char* GetDebugPanelName() const override {return "Player Deck Management";}
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;
	
	UWorld* World = nullptr;
	TArray<FAssetData> CacheCardAssets();
	
private:
	
	static TArray<FAssetData> CachedCardDataAssets;
	static TSoftObjectPtr<UCardDataAsset> SelectedCardAsset;
	static UDeckManagerComponent* DeckManagerComponent;
	
	static TMap<TWeakObjectPtr<UTexture>, FImGuiTextureHandle> TextureHandles;
	
	static FImGuiTextureHandle GetOrRegisterTexture(UTexture* Texture);
	static float GetOrAddCardLevelOverride(UBaseCardGameplayAbilitySet* CardSet);
	
	static TMap<TWeakObjectPtr<UBaseCardGameplayAbilitySet>, float> CardLevelOverrides;
};
