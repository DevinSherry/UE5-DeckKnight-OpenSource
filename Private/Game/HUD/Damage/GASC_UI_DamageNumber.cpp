// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HUD/Damage/GASC_UI_DamageNumber.h"
#include "Components/TextBlock.h"

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
	// Basic safety checks
	if (!DamageText || !DamageTypeUIData)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumber: Missing DamageText or DamageTypeUIData."));
		return;
	}

	// Pipeline type (Damage vs Healing) now comes from the unified damage pipeline
	const bool bIsHealing = (DamageModContext.DamagePipelineType == EGASC_DamagePipelineType::Healing);
	const float RawValue   = DamageModContext.DeltaValue;
	const int32 Rounded    = FMath::RoundToInt(RawValue);
	if (Rounded == 0)
	{
		return;
	}

	// Choose sign / text formatting
	if (bIsHealing)
	{
		// Positive sign for healing
		DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Rounded)));
	}
	else
	{
		DamageText->SetText(FText::AsNumber(Rounded));
	}

	// Color by damage type tag coming from the pipeline context
	FGameplayTag DamageTypeTag = DamageModContext.DamageType;
	if (!DamageTypeTag.IsValid())
	{
		// If type isn't specified, keep default color (gray-ish)
		DamageText->SetColorAndOpacity(FLinearColor::Gray);
		return;
	}

	const FLinearColor Color = DamageTypeUIData->GetDamageTypeColor(DamageTypeTag);
	DamageText->SetColorAndOpacity(Color);
}

void UGASC_UI_DamageNumber::SetCriticalHitText_Implementation()
{
	if (!DamageTypeUIData)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumber: DamageTypeUIData is NULL in SetCriticalHitText."));
		return;
	}

	if (DamageText)
	{
		DamageText->SetColorAndOpacity(DamageTypeUIData->CriticalDamageColor);
		DamageText->SetText(DamageTypeUIData->CriticalDamageText);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageNumber: DamageText is NULL in SetCriticalHitText."));
	}
}
