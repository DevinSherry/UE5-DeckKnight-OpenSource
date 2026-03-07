// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Blueprint/UserWidget.h"
#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"
#include "GASC_UI_CardTemplate.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_UI_CardTemplate : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ActivateCard();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void InstantiateCardData();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Information")
	class UCardDataAsset* CardData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Information")	
	FCardInstance CardInstance;
};
