// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Card/CardDataAsset.h"

#if WITH_EDITOR
EDataValidationResult UCardDataAsset::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;
	return Result;
}
#endif
