// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/GameplayAbilitySystem/AttributeSets/GASCourseAttributeSet.h"
#include "GASC_DeckAttributeSet.generated.h"

/**
 * @class UGASC_DeckAttributeSet
 * @brief Represents a set of attributes specific to a deck in the system.
 *
 * The UGASC_DeckAttributeSet class encapsulates and manages attributes
 * related to a deck. It is designed to provide functionality for attribute
 * handling, including initialization and modification specific to the deck's
 * requirements.
 *
 * This class may typically be used within gameplay mechanics or to define
 * unique properties of decks in the system. It extends base attribute functionalities
 * and provides customization points specific to deck-related attributes.
 *
 * Key responsibilities of this class include:
 * - Managing deck-specific attributes.
 * - Defining default values for those attributes.
 * - Providing methods to interact with and modify attributes safely.
 *
 * This class ensures robust handling of deck attributes to maintain gameplay integrity
 * and facilitate flexibility in attribute management.
 */
UCLASS()
class GASCOURSE_API UGASC_DeckAttributeSet : public UGASCourseAttributeSet
{
	GENERATED_BODY()

public:

	UGASC_DeckAttributeSet();

	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Character Deck Attributes")
	FGameplayAttributeData MaxHandCount;
	ATTRIBUTE_ACCESSORS(UGASC_DeckAttributeSet, MaxHandCount)

	UPROPERTY(BlueprintReadOnly, Category = "Character Deck Attributes")
	FGameplayAttributeData MaxDeckCount;
	ATTRIBUTE_ACCESSORS(UGASC_DeckAttributeSet, MaxDeckCount)
};
