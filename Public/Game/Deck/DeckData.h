// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Card/BaseCardGameplayAbilitySet.h"
#include "DeckData.generated.h"

/**
 * @class UDeckData
 * @brief A data class that provides representation and handling of deck-related data within the context of the GAS Course project.
 *
 * UDeckData serves as a UObject-based class for encapsulating all deck-related information and functionality.
 * This class is designed to be extended or utilized wherever deck-specific data management is required.
 *
 * It is part of the GAS Course API and functions as a modular unit for handling deck logic, enabling clear,
 * organized, and scalable implementation of card or deck mechanics.
 *
 * @note This class is intended to work within Unreal Engine's UObject system and follows its lifecycle and design paradigms.
 */

UENUM(BlueprintType)
enum ECardType
{
	AbilityCard,
	PassiveAbilityCard,
	GlobalPassiveCard,
	ConsumableCard,
	EmptyCard
};

USTRUCT(BlueprintType)
struct FCardData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASCourse|Card Data")
	FText CardName;

	/*
	 * This needs to be more than just an FText. It needs to have dynamic values written into the description based on current player attributes & modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASCourse|Card Data")
	FText CardDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASCourse|Card Data")
	TEnumAsByte<ECardType> CardType = ECardType::EmptyCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASCourse|Card Data")
	UBaseCardGameplayAbilitySet* CardAbilitySet = nullptr;
	
};