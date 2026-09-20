// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GASC_UI_CardTemplate.h"
#include "Blueprint/UserWidget.h"
#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"
#include "GASC_UI_CardHand.generated.h"

/**
 * @class UGASC_UI_CardHand
 * @brief Represents a UI component for managing and displaying a hand of cards in a game.
 *
 * This class is responsible for organizing, displaying, and updating a hand of cards
 * in the user interface. It provides functionality to add, remove, and arrange cards
 * visually while maintaining proper alignment and animation for dynamic adjustments.
 *
 * The UGASC_UI_CardHand class interacts with individual card UI elements to reflect
 * game state changes and enhances the player's visual experience through smooth
 * transitions and responsive animations when the hand is updated.
 *
 * Responsibilities of this class include:
 * - Managing the layout and order of card elements in the hand.
 * - Providing functionality for adding and removing card visuals.
 * - Supporting dynamic UI updates with animations.
 * - Handling user interactions associated with the card hand, if applicable.
 *
 * This class is designed to integrate with the game's overall UI framework, and
 * it supports customization of card-handling behavior such as layout styles or
 * animation patterns.
 */
UCLASS()
class GASCOURSE_API UGASC_UI_CardHand : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void NativeConstruct() override;
	
public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(BindWidget))
	class UCanvasPanel* CardHandCanvas;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Template")
	TSubclassOf<UGASC_UI_CardTemplate> CardTemplateClass;
	
public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	int32 DebugForceNumberofCards = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Card Hand Settings")
	float MinimumCardSpacing = 0.3f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float MaximumCardSpacing = 0.5f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float CardScale = 0.5f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float AnimationSpeed = 12.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float CardAngle = 21.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float HandWidthPercentage = 1.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float CardArcAmount = 0.5f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float ExtraYOffset = -300.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float SelectedCardScale = 1.05f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float SelectedShowXAmount = 0.85f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	float SelectedShowYAmount = 0.4f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	bool bKeepStackOnLeft = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	bool bDoStackCardsOnSides = true;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	bool bSelectNextCardOnUse = true;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Card Hand Settings")
	bool bStraightSelectedCard = true;
	
	UFUNCTION(BlueprintImplementableEvent)
	void AddCardToHand_Event();
	
	UFUNCTION(BlueprintCallable)
	void UpdateCardPositions(float DeltaTime);
	
	UFUNCTION(BlueprintCallable)
	FWidgetTransform CalculateSelectedCardTransform_Native(UGASC_UI_CardTemplate* SelectedCard, FWidgetTransform TargetTransform);
	
	UFUNCTION(BlueprintCallable)
	FWidgetTransform CalculateCardTransform_Native(UGASC_UI_CardTemplate* CurrentCard, int32 CurrentCardIndex, int32 TotalNumberOfCards);
	
	UFUNCTION(BlueprintCallable)
	FWidgetTransform InterpolateCardTransform_Native(FWidgetTransform CurrentTransform, FWidgetTransform TargetTransform, float DeltaTime);
	
	UFUNCTION()
	float CalculateYOffset_Native() const;
	
	UFUNCTION(BlueprintCallable)
	float CalculateCardXTranslation_Native(UGASC_UI_CardTemplate* CurrentCard, int32 CurrentCardIndex, int32 TotalNumberOfCards) const;
	
	UFUNCTION()
	float CalculateCardYTranslation_Native(TWeakObjectPtr<UGASC_UI_CardTemplate> CurrentCard, int32 CurrentCardIndex, int32 TotalNumberOfCards) const;
	
	UFUNCTION()
	float CalculateCardAngle_Native(int32 CurrentCardIndex, int32 TotalNumberOfCards) const;

	UFUNCTION(BlueprintCallable)
	void AddCardToHand();
	
	UFUNCTION(BlueprintCallable)
	void AddCardToHand_WithData(UCardDataAsset* CardData);
	
	UFUNCTION(BlueprintCallable)
	void AddCardInstanceToHand(FCardInstance CardInstanceData);
	
	UFUNCTION(BlueprintCallable)
	void RemoveCardFromHand();
	
	UFUNCTION(BlueprintCallable)
	int32 GetNumberOfCardsInHand() const;
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card Hand")
	TArray<UGASC_UI_CardTemplate*> CardsInHand;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card Hand")
	int32 SelectedCardIndex;

	/**
	 * @property DeckManager
	 * @brief Manages and interacts with the deck system for a character.
	 *
	 * DeckManager is a component responsible for handling operations related to the deck of cards associated with a character.
	 * It provides functionalities to manage active cards, discarded cards, and the player's hand, ensuring seamless integration
	 * with gameplay mechanics and UI systems. This property serves as the primary interface for the character to interact
	 * with card-based functionalities.
	 *
	 * Responsibilities of DeckManager include:
	 * - Managing the lifecycle of the active deck, discard pile, and player's hand.
	 * - Facilitating card transitions between different states (e.g., from deck to hand or hand to discard).
	 * - Supporting extensions for gameplay logic, such as reshuffling, interactions, and deck customization.
	 *
	 * This property can be modified within the editor and accessed in Blueprints for flexibility and integration with the
	 * game's systems. It is categorized under "Character|Components" for organizational purposes and encapsulates private
	 * access control to comply with encapsulation principles.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Components", meta = (AllowPrivateAccess = "true"))
	UDeckManagerComponent* DeckManager = nullptr;

	/**
	 * @property AbilitySystemComponent
	 * @brief Represents the core ability system component used for managing abilities and gameplay effects in a character.
	 *
	 * The AbilitySystemComponent is responsible for handling the application, execution, and management of gameplay abilities
	 * and effects in the context of the character it is attached to. It serves as the primary interface for gameplay ability
	 * system interactions, enabling functionality such as attribute management, gameplay tag handling, and ability activations.
	 *
	 * Key responsibilities of this component include:
	 * - Managing the lifecycle of gameplay abilities, including activation, cooldowns, and cost calculations.
	 * - Handling gameplay effects, such as buffs, debuffs, temporary attribute changes, and persistent health or mana modifications.
	 * - Supporting gameplay tags for complex rules, such as blocking or requiring specific conditions for ability execution.
	 * - Providing an extensible framework for integrating additional gameplay-related logic.
	 *
	 * This property is categorized under "Character|Components" for organizational purposes in the editor. It is marked as private
	 * access to ensure encapsulation and is editable and accessible in Blueprints for additional customization. The component is expected
	 * to integrate with the overall Gameplay Ability System (GAS) provided by Unreal Engine.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Components", meta = (AllowPrivateAccess = "true"))
	UGASCourseAbilitySystemComponent* AbilitySystemComponent = nullptr;
};
