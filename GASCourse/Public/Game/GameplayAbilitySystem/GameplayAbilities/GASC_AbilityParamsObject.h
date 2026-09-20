// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructUtils/PropertyBag.h"
#include "GASC_AbilityParamsObject.generated.h"

GASCOURSE_API DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_AbilityParams, Log, All);

/**
 * Carries a bag of ability parameter values from whoever granted an ability to the ability itself.
 *
 * Granting code fills the bag and hands this object to FGameplayAbilitySpec as its SourceObject;
 * the ability then calls ApplyTo on its own instance to push those values into its matching
 * properties. FGrantAbilityDynamicTask is the main producer - it reflects the ability class into
 * bindable State Tree pins and packs the results in here.
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

	/**
	 * Copies every bag value into the like-named property on Target.
	 *
	 * Each copy is type-checked first. A bag and the class it was reflected from can drift apart -
	 * a property gets retyped, or the asset holding the bag was saved against an older version of
	 * the class - and the raw byte copy underneath has no way to notice. A mismatch is skipped and
	 * reported rather than allowed to reinterpret one type's bytes as another's.
	 *
	 * Safe to call more than once for the same ability; it is idempotent.
	 *
	 * @return the number of properties actually written.
	 */
	int32 ApplyTo(UObject* Target) const;
};
