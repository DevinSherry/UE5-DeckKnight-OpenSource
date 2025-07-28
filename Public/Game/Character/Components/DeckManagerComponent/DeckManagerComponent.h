// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Card/CardDataAsset.h"
#include "DeckManagerComponent.generated.h"


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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	TArray<UCardDataAsset*> ActiveDeck;

	UFUNCTION(BlueprintCallable, Category = "Deck")
	bool ActivateCard(UCardDataAsset* CardToActivate);
	
};
