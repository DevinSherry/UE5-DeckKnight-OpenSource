// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASC_CardHandUILayoutDebug.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

float FGASC_CardHandUILayoutDebug::CardSpacing = 10.0f;
float FGASC_CardHandUILayoutDebug::CardScale = 10.0f;

FGASC_CardHandUILayoutDebug::FGASC_CardHandUILayoutDebug()
{
}

FGASC_CardHandUILayoutDebug::~FGASC_CardHandUILayoutDebug()
{
}

void FGASC_CardHandUILayoutDebug::DrawDebugPanel(bool& bOpen)
{
	if (!bOpen)
		return;

	if (!ImGui::Begin("Card Hand Layout Modifier Properties", &bOpen))
	{
		ImGui::End();
		return;
	}
	
	if (!World)
	{
		for (TWeakObjectPtr<APawn> Pawn : CachedPawns)
		{
			if (Pawn.Get() && Pawn.Get()->GetWorld())
			{
				World = Pawn.Get()->GetWorld();
			}
		}
	}
	if (!CardHand)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UGASC_UI_CardHand::StaticClass(), false);

		for (UUserWidget* FoundWidget : FoundWidgets)
		{
			if (UGASC_UI_CardHand* FoundCardHand = Cast<UGASC_UI_CardHand>(FoundWidget))
			{
				CardHand = FoundCardHand;
				CardSpacing = CardHand->CardSpacing;
				CardScale = CardHand->CardScale;
				break;
			}
		}
	}
	
	if (CardHand)
	{
		ImGui::SliderFloat("Card Spacing", &CardSpacing, 0.0f, 100.0f);
		CardHand->CardSpacing = CardSpacing;
		
		ImGui::SliderFloat("Card Scale", &CardScale, 0.5f, 2.0f);
		CardHand->CardScale = CardScale;
	}

	ImGui::End();
}

void FGASC_CardHandUILayoutDebug::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}
