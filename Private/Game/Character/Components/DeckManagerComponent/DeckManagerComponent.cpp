// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Components/DeckManagerComponent/DeckManagerComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace GASCourse_DeckManagerComponentCVars
{
	static bool bIgnoreCardCosts = false;
	FAutoConsoleVariableRef CvarIgnoreCardCost(
		TEXT("GASCourseDebug.DeckManagerComponent.IgnoreCardCosts"),
		bIgnoreCardCosts,
		TEXT("Ignore costs of cards. Card resource is not spent, and you can activate any card, regardless of cost (Enabled: true, Disabled: false)"));
	
}

// Sets default values for this component's properties
UDeckManagerComponent::UDeckManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UDeckManagerComponent::DebugIgnoreCardCostEnabled()
{
	return GASCourse_DeckManagerComponentCVars::bIgnoreCardCosts;
}

void UDeckManagerComponent::SetDebugIgnoreCardCost(bool bIgnoreCardCost)
{
	GASCourse_DeckManagerComponentCVars::bIgnoreCardCosts = bIgnoreCardCost;
}

// Called when the game starts
void UDeckManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystemComponent = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
	
	// ...
	
}

bool UDeckManagerComponent::ActivateCardByInstanceID(const FGuid& CardInstanceID)
{
	
	FCardInstance* Card = CurrentHandInstance.FindByPredicate(
		[&](const FCardInstance& C){ return C.CardInstanceId == CardInstanceID; });
	
	if (!Card)
	{
		UE_LOGFMT(LogTemp, Warning, "Attempted to activate card with instance ID {0}}, but it was not found in the current hand.", *CardInstanceID.ToString());
		return false;
	}
	
	UCardDataAsset* CardAsset = Card->CardDataAsset.LoadSynchronous();
	if (!CardAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Card asset was not found for instance ID %s"), *CardInstanceID.ToString());
		return false;
	}
	
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent is NULL"));
		return false;
	}
	
	OnCardActivated.Broadcast(*Card);
	UE_LOG(LogTemp, Warning, TEXT("Card level activated: %i"), Card->CardLevel);
	CardAsset->CardData.CardAbilitySet->ActivateCard(AbilitySystemComponent, Card->CardLevel);
	return true;
}

FCardInstance UDeckManagerComponent::DrawCardInstance()
{
	FCardInstance CardInstanceToDraw;
	if (ActiveDeckInstance.IsEmpty())
	{
		UE_LOGFMT(LogTemp, Warning, "Attempted to draw card from an empty active deck, early exit.");
		return CardInstanceToDraw;
	}
	CardInstanceToDraw = ActiveDeckInstance.Pop();
	CurrentHandInstance.Add(CardInstanceToDraw);
	CardInstanceToDraw.CardDataAsset = CardInstanceToDraw.CardDataAsset.LoadSynchronous();
	OnCardAddedToHand.Broadcast(CardInstanceToDraw);
	
	return CardInstanceToDraw;
}

void UDeckManagerComponent::ForceCardInstanceInHand(FCardInstance CardInstanceToAdd)
{
	if (!CardInstanceToAdd.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Card to add to hand is NULL"));
		return;
	}
	CurrentHandInstance.Add(CardInstanceToAdd);
	OnCardAddedToHand.Broadcast(CardInstanceToAdd);
}

void UDeckManagerComponent::FindAllCards(TArray<FAssetData>& OutAllCards)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	
	Filter.ClassPaths.Add(UCardDataAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName("/Game"));
	
	AssetRegistry.GetAssets(Filter, OutAllCards);
}

