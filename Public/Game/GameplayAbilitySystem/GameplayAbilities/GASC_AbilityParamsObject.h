// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructUtils/PropertyBag.h"
#include "GASC_AbilityParamsObject.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GASCOURSE_API UGASC_AbilityParamsObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FInstancedPropertyBag Params;

	void InitFromBag(const FInstancedPropertyBag& InBag)
	{
		Params = InBag; // deep copy
	}
};
