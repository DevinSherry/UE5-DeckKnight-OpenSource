// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "GASC_UI_CardHand.generated.h"

/**
 * 
 */
UCLASS()
class GASCOURSE_API UGASC_UI_CardHand : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float CardSpacing = 10.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float CardScale = 0.5f;
};
