// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
};
