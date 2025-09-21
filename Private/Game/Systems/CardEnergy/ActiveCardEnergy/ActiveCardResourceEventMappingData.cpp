// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/CardEnergy/ActiveCardEnergy/ActiveCardResourceEventMappingData.h"

DEFINE_LOG_CATEGORY(LOG_GASC_ActiveCardResourceMapping);

float UActiveCardResourceEventMappingData::GetGameplayEventMapping(FGameplayTag GameplayEventTag, bool& bValueFound, const float InLevelValue) const
{
	bValueFound = false;
	if (GameplayEventTag.IsValid())
	{
		const FScalableFloat* MappingValue = GameplayEventMapping.Find(GameplayEventTag);
		if (!MappingValue)
		{
			UE_LOG(LOG_GASC_ActiveCardResourceMapping, Verbose, TEXT("Invalid Mapping Value for tag %s. Returning 0.0f."),
				*GameplayEventTag.GetTagName().ToString());
			return 0.0f;
		}
		
		float MappingValueFloat = MappingValue->GetValueAtLevel(InLevelValue);
		UE_LOG(LOG_GASC_ActiveCardResourceMapping, Log, TEXT("Gameplay Event Tag: %s | Mapping Value: %f"),
			*GameplayEventTag.GetTagName().ToString(), MappingValueFloat);
		bValueFound = true;
		return MappingValueFloat;
	}
	
	UE_LOG(LOG_GASC_ActiveCardResourceMapping, Verbose, TEXT("Invalid Gameplay Event Tag. Returning 0.0f."));
	return 0.0f;
}
