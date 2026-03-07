// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "Game/Deck/DeckData.h"
#include "CardDataAsset.generated.h"

/**
 * @class UCardDataAsset
 * @brief A data asset class used for storing and managing card-related data in a game or application.
 *
 * The UCardDataAsset class provides a blueprintable way to define and store
 * metadata, properties, and functionalities required for individual cards. This
 * can include attributes like card name, description, abilities, values, or
 * any other custom properties used in gameplay or UI representation.
 *
 * This class is typically used as part of a data-driven design to decouple
 * design data from code, making it easier to configure and manage card-specific
 * information.
 *
 * @note This class should be inherited to extend its functionality or specify
 * additional fields.
 *
 * Key features:
 * - Acts as a container for card data.
 * - Supports extension for additional card-related functionality.
 * - Blueprint-friendly for ease of use in Unreal Engine.
 */
UCLASS()
class GASCOURSE_API UCardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FCardData CardData;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
};
