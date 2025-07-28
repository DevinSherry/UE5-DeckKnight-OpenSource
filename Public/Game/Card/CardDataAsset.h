// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "CardDataAsset.generated.h"

/**
 *
 */
UCLASS()
class GASCOURSE_API UCardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FDataTableRowHandle CardDataRowHandle;

	UFUNCTION()
	bool ActivateCard(AActor* Instigator = nullptr);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
};
