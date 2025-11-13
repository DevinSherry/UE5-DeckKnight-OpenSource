// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUD/Damage/GASC_UI_DamageNumber.h"
#include "Components/TextBlock.h"
#include "Game/BlueprintLibraries/GameplayAbilitySystem/GASCourseASCBlueprintLibrary.h"


void UGASC_UI_DamageNumber::NativeConstruct()
{
	Super::NativeConstruct();
}

void UGASC_UI_DamageNumber::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UGASC_UI_DamageNumber::SetDamageTextValue_Implementation()
{
	if (!DamageText)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageText is NULL"));
		return;
	}

	if (!DamageData.Instigator)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageData.Instigator is NULL"));
		return;
	}

	if (!DamageTypeUIData)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageTypeUIData is NULL"));
		return;
	}

	float DamageValue = DamageData.EventMagnitude;

	if (DamageData.InstigatorTags.HasTag(DamageType_Healing))
	{
		FText HealingText = FText::FromString(FString::Printf(TEXT("+%.0f"), DamageValue));
		DamageText->SetText(HealingText);
	}
	else
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageValue)));
	}

	FLinearColor DamageTextColor = FLinearColor::Gray;

	FGameplayTag DamageTypeTag = FGameplayTag::EmptyTag;
	UGASCourseASCBlueprintLibrary::FindDamageTypeTagInContainer(DamageData.InstigatorTags, DamageTypeTag);

	if (!DamageTypeTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageTypeTag is INVALID"));
		return;
	}

	DamageTextColor = DamageTypeUIData->GetDamageTypeColor(DamageTypeTag);
	DamageText->SetColorAndOpacity(DamageTextColor);
}

void UGASC_UI_DamageNumber::SetCriticalHitText_Implementation()
{
	if (!DamageTypeUIData)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageTypeColorData is NULL"));
		return;
	}
	if (DamageText)
	{
		DamageText->SetColorAndOpacity(DamageTypeUIData->CriticalDamageColor);
		DamageText->SetText(DamageTypeUIData->CriticalDamageText);
	}
}
