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

USTRUCT(BlueprintType)
struct FCardInstance
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, SaveGame)
	FGuid CardInstanceId = FGuid::NewGuid();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	TSoftObjectPtr<UCardDataAsset> CardDataAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	int32 CardLevel = 1;
	
	bool IsValid() const {return CardInstanceId.IsValid() && CardDataAsset.IsValid();}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeckManager_OnCardActivated, const FCardInstance&, Card);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeckManager_OnCardAddedToHand, const FCardInstance&, Card);

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
	 * Represents the active set of cards currently in play within the deck system.
	 *
	 * This array is managed by the deck handling component to track and store instances of cards
	 * that are actively in use during gameplay or processing. It enables operations like drawing,
	 * updating, or manipulating cards that are part of the active deck.
	 *
	 * Designed to work seamlessly within the Unreal Engine framework, this variable supports both
	 * Blueprint and C++ access for efficient integration and flexibility in deck-related systems.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deck")
	TArray<FCardInstance> ActiveDeckInstance;

	/**
	 * An array that stores the collection of card instances managed by the deck system.
	 *
	 * Represents the primary container for cards within a deck, supporting functionalities such as modifications,
	 * shuffling, and retrieval of card data. This array is primarily used in conjunction with the deck management
	 * systems to facilitate game mechanics or application features involving card operations.
	 *
	 * Accessible via both Blueprint and C++ for flexibility in game or application development workflows.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deck")
	TArray<FCardInstance> CardBinderInstance;

	/**
	 * Represents the current set of cards held in the player's hand within the deck management system.
	 *
	 * This array contains instances of FCardInstance, each of which corresponds to an individual card currently in play
	 * as part of the active hand. It is primarily used to track cards that have been drawn from the deck but not yet
	 * returned, discarded, or otherwise removed from play.
	 *
	 * The property is accessible both in the editor and during runtime, allowing designers and developers to inspect
	 * and modify the current hand state via Blueprint or other tools.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deck")
	TArray<FCardInstance> CurrentHandInstance;

	/**
	 * Represents the collection of discarded card instances within the deck system.
	 *
	 * This array tracks card instances that have been moved to the graveyard during gameplay.
	 * It provides an organized structure to store and manage cards that are no longer active
	 * in the player's hand or main deck but are retained for future processing or reference.
	 *
	 * Typically used to maintain the state of the graveyard for game rules or mechanics that
	 * interact specifically with discarded cards. Accessible via Blueprint and C++ for
	 * enhanced customization and gameplay integration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deck")
	TArray<FCardInstance> DeckGraveyardInstance;

public:
	
	/**
	 * Activates a card in the system based on its unique instance ID.
	 *
	 * This method interacts with the card management system to locate a card using its instance ID
	 * and triggers its activation. Activation may involve executing specific behaviors or effects
	 * associated with the card, updating its state, and integrating it into the active gameplay logic.
	 *
	 * @param CardInstanceID The unique identifier of the card instance to be activated.
	 * @return A boolean value indicating whether the card was successfully activated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	bool ActivateCardByInstanceID(const FGuid& CardInstanceID);

	/**
	 * Draws a card from the active deck and adds it to the current hand instance.
	 *
	 * This method retrieves a card instance from the active deck, ensuring that the drawn card's necessary data is synchronized
	 * before being added to the current hand. If the active deck is empty, a warning is logged, and an empty card instance is returned.
	 * Additionally, this function triggers a broadcast event upon successfully adding a card to the hand.
	 *
	 * @return The drawn FCardInstance object, or an empty instance if no cards are available in the active deck.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	FCardInstance DrawCardInstance();

	/**
	 * Forces a specified card instance to be added directly to the current hand.
	 *
	 * This method adds the provided card instance to the hand, bypassing standard deck or draw mechanics.
	 * It ensures that the card is placed in the player's hand immediately if it is valid.
	 * A broadcast event is triggered upon successful addition of the card to notify listeners.
	 *
	 * @param CardInstanceToAdd The card instance to forcibly add to the current hand. The card must be valid; otherwise, the operation will fail.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void ForceCardInstanceInHand(FCardInstance CardInstanceToAdd);
	
	/**
	 * The FindAllCards method retrieves a collection of all card entities within the system.
	 *
	 * This method performs a search through the relevant card data structures or repositories and returns all available cards.
	 * It is designed to be used for scenarios where the application needs to process or display a comprehensive list of cards.
	 *
	 * @param OutAllCards The reference to the container or list where all the retrieved cards will be stored.
	 */
	UFUNCTION(BlueprintCallable)
	static void FindAllCards(TArray<FAssetData>& OutAllCards);
	
	//TODO: Add helper functions for both graveyard and active decks
	/**
	 * Deck count -> remaining cards in both active and graveyard decks
	 * How many cards of type remaining in both active and graveyard decks
	 * Add card from graveyard to hand
	 * Add card from graveyard to active deck
	 * Add card from hand to graveyard
	 * Add card from hand back into active deck (randomized)
	 */
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess = true))
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;
	
private:
	/**
	 * Delegate triggered when a card is activated within the deck system.
	 *
	 * This delegate is primarily used to signal when a specific card in the deck becomes active,
	 * allowing other components or systems to respond to the activation event. It enables dynamic
	 * interactions and custom functionality tied to card activation, fostering modular and event-driven
	 * design in the application.
	 *
	 * Accessible through both C++ and Blueprint integrations, the delegate supports robust communication
	 * between gameplay systems and the deck manager component.
	 */
	UPROPERTY(BlueprintAssignable, Category="Deck|Card")
	FDeckManager_OnCardActivated OnCardActivated;

	/**
	 * Delegate triggered when a card is added to the player's hand in the Deck Manager component.
	 *
	 * This event is invoked whenever a card is successfully moved to the player's hand, enabling gameplay systems or UI
	 * elements to respond to the addition of the card. It allows for customization of game-specific logic tied to card-drawing
	 * mechanics or player interactions.
	 *
	 * The delegate is categorized under "Deck|Card" for better organization and is assignable via Blueprints, ensuring flexibility
	 * when integrating it into various gameplay scenarios.
	 */
	UPROPERTY(BlueprintAssignable, Category="Deck|Card")
	FDeckManager_OnCardAddedToHand OnCardAddedToHand;
	
};
