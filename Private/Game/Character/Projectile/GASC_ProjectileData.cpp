// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Character/Projectile/GASC_ProjectileData.h"

#if WITH_EDITOR
#include "Engine/CollisionProfile.h"
#endif

#if WITH_EDITOR
TArray<FName> UGASC_ProjectileData::GetCollisionProfileOptions() const
{
	TArray<FName> CollisionProfiles;
	
	const UCollisionProfile* CollisionProfile = UCollisionProfile::Get();
	if (!CollisionProfile)
	{
		return CollisionProfiles;
	}
	
	TArray<TSharedPtr<FName>> CollisionProfileNames;
	CollisionProfile->GetProfileNames(CollisionProfileNames);
	for (const TSharedPtr<FName>& ProfileName : CollisionProfileNames)
	{
		CollisionProfiles.Add(*ProfileName);
	}
	
	return CollisionProfiles;
}
#endif
