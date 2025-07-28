// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Card/CardDataAsset.h"
#include "Game/Deck/DeckData.h"
#include "Misc/DataValidation.h"

bool UCardDataAsset::ActivateCard(AActor* Instigator)
{
	bool bSuccess = false;

	if (CardDataRowHandle.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("Card Data Row Handle is Null"));
		return bSuccess;
	}

	if (Instigator == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Instigator is NULL"));
		return bSuccess;
	}
	
	TObjectPtr<const UDataTable> CardDataTable =  CardDataRowHandle.DataTable;
	FName CardDataRowName = CardDataRowHandle.RowName;
	AGASCoursePlayerState* InstigatorState = Cast<AGASCoursePlayerState>(Instigator);
	
	if (!InstigatorState)
	{
		UE_LOG(LogTemp, Error, TEXT("Instigator is not a Character"));
		return bSuccess;
	}

	UGASCourseAbilitySystemComponent* AbilitySystemComponent = InstigatorState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability System Component is NULL"));
		return bSuccess;
	}
	
	if (IsValid(CardDataTable))
	{
		if (const FCardData* CardData = CardDataTable->FindRow<FCardData>(CardDataRowName, TEXT("Looking Up Card")))
		{
			FGASCourseAbilitySet_GrantedHandles* OutGrantedHandles = new FGASCourseAbilitySet_GrantedHandles();
			CardData->CardAbilitySet->ActivateCard(AbilitySystemComponent, OutGrantedHandles);
			bSuccess = true;
		}
	}
	
	return bSuccess;
}

#if WITH_EDITOR
EDataValidationResult UCardDataAsset::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;
	if (CardDataRowHandle.IsNull())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString("Card Data Row Handle is not set"));
	}

	return Result;
}
#endif
