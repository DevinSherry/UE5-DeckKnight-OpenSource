// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Card/CardDataAsset.h"
#include "DeckManagerComponent.generated.h"

/**
 * The UDeckManagerComponent is responsible for managing the deck system within the application.
 *
 * This component handles operations related to creating, modifying, shuffling, and maintaining card decks.
 * It serves as a central module to manage active, inactive, and discard decks, ensuring seamless integration and functionality
 * of the deck-processing logic in-game or an application environment.
 *
 * The class is designed to work with collections of UCardDataAsset objects, providing functionality via Blueprint and C++ APIs.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GASCOURSE_API UDeckManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDeckManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/**
	 * Represents the active deck of cards in the Deck Manager component.
	 *
	 * This array contains UCardDataAsset references, which represent the card data currently included in the active deck.
	 * The variable is editable in the editor and can be accessed read-only in Blueprints. It is categorized under "Deck" for organizational purposes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	TArray<UCardDataAsset*> ActiveDeck;

	/**
	 * Represents the graveyard of discarded cards managed by the Deck Manager component.
	 *
	 * This array holds references to UCardDataAsset objects, which represent cards that have been moved out of active play
	 * and into the discard or "graveyard" pile. It facilitates the tracking of removed cards and provides a mechanism
	 * for extending gameplay logic based on discarded cards, such as reshuffling or special interactions.
	 *
	 * The variable is editable in the editor and can be accessed and modified via Blueprints. It is categorized under "Deck"
	 * for better organization and clarity within the editor or game systems.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	TArray<UCardDataAsset*> DeckGraveyard;

public:
	
	UFUNCTION(BlueprintCallable, Category = "Deck")
	bool ActivateCard(UCardDataAsset* CardToActivate);

	//TODO: Add helper functions for both graveyard and active decks
	/**
	 * Deck count -> remaining cards in both active and graveyard decks
	 * How many cards of type remaining in both active and graveyard decks
	 * Add card from graveyard to hand
	 * Add card from graveyard to active deck
	 * Add card from hand to graveyard
	 * Add card from hand back into active deck (randomized)
	 */
	
};
