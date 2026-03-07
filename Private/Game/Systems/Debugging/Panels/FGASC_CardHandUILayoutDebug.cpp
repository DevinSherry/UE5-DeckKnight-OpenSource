// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASC_CardHandUILayoutDebug.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "imgui.h"

float FGASC_CardHandUILayoutDebug::MaximumCardSpacing = 0.5f;
float FGASC_CardHandUILayoutDebug::MinimumCardSpacing = 0.3f;
float FGASC_CardHandUILayoutDebug::AnimationSpeed = 12.0f;
float FGASC_CardHandUILayoutDebug::CardScale = 0.5f;
float FGASC_CardHandUILayoutDebug::CardAngle = 21.0f;
float FGASC_CardHandUILayoutDebug::HandWidthPercentage = 1.0f;
float FGASC_CardHandUILayoutDebug::CardArcAmount = 0.5f;
float FGASC_CardHandUILayoutDebug::ExtraYOffset = -300.0f;
float FGASC_CardHandUILayoutDebug::SelectedCardScale = 1.05f;
float FGASC_CardHandUILayoutDebug::SelectedShowXAmount = 0.85f;
float FGASC_CardHandUILayoutDebug::SelectedShowYAmount = 0.4f;

bool FGASC_CardHandUILayoutDebug::bKeepStackOnLeft = false;
bool FGASC_CardHandUILayoutDebug::bDoStackCardsOnSides = true;
bool FGASC_CardHandUILayoutDebug::bSelectNextCardOnUse = true;

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
				MinimumCardSpacing = CardHand->MinimumCardSpacing;
				MaximumCardSpacing = CardHand->MaximumCardSpacing;
				CardScale = CardHand->CardScale;
				AnimationSpeed = CardHand->AnimationSpeed;
				CardAngle = CardHand->CardAngle;
				HandWidthPercentage = CardHand->HandWidthPercentage;
				CardArcAmount = CardHand->CardArcAmount;
				ExtraYOffset = CardHand->ExtraYOffset;
				SelectedCardScale = CardHand->SelectedCardScale;
				SelectedShowXAmount = CardHand->SelectedShowXAmount;
				SelectedShowYAmount = CardHand->SelectedShowYAmount;
				bKeepStackOnLeft = CardHand->bKeepStackOnLeft;
				bDoStackCardsOnSides = CardHand->bDoStackCardsOnSides;
				bSelectNextCardOnUse = CardHand->bSelectNextCardOnUse;
				break;
			}
		}
	}
	
	if (CardHand)
	{
		if (ImGui::SliderFloat("Minimum Card Spacing", &MinimumCardSpacing, 0.0f, 20.0f))
		{
			CardHand->MinimumCardSpacing = MinimumCardSpacing;
		}
		
		if (ImGui::SliderFloat("Maximum Card Spacing", &MaximumCardSpacing, 0.0f, 20.0f))
		{
			CardHand->MaximumCardSpacing = MaximumCardSpacing;
		}
		
		if (ImGui::SliderFloat("Card Scale", &CardScale, 0.1f, 2.0f))
		{
			CardHand->CardScale = CardScale;
		}
		
		if (ImGui::SliderFloat("Animation Speed", &AnimationSpeed, 1.0f, 30.0f))
		{
			CardHand->AnimationSpeed = AnimationSpeed;
		}
		
		if (ImGui::SliderFloat("Card Angle", &CardAngle, 0.0f, 90.0f))
		{
			CardHand->CardAngle = CardAngle;
		}
		
		if (ImGui::SliderFloat("Hand Width Percentage", &HandWidthPercentage, 0.0f, 2.0f))
		{
			CardHand->HandWidthPercentage = HandWidthPercentage;
		}
		
		if (ImGui::SliderFloat("Card Arc Amount", &CardArcAmount, 0.01f, 10.0f))
		{
			CardHand->CardArcAmount = CardArcAmount;
		}
		
		if (ImGui::SliderFloat("Extra Y Offset", &ExtraYOffset, -500.0f, 500.0f))
		{
			CardHand->ExtraYOffset = ExtraYOffset;
		}
		
		if (ImGui::SliderFloat("Selected Card Scale", &SelectedCardScale, 0.0f, 2.0f))
		{
			CardHand->SelectedCardScale = SelectedCardScale;
		}
		
		if (ImGui::SliderFloat("Selected Show X Amount", &SelectedShowXAmount, 0.0f, 2.0f))
		{
			CardHand->SelectedShowXAmount = SelectedShowXAmount;
		}
		
		if (ImGui::SliderFloat("Selected Show Y Amount", &SelectedShowYAmount, 0.0f, 2.0f))
		{
			CardHand->SelectedShowYAmount = SelectedShowYAmount;
		}
		
		if (ImGui::Checkbox("Keep Stack on Left", &bKeepStackOnLeft))
		{
			CardHand->bKeepStackOnLeft = bKeepStackOnLeft;
		}
		if (ImGui::Checkbox("Stack Cards On Sides", &bDoStackCardsOnSides))
		{
			CardHand->bDoStackCardsOnSides = bDoStackCardsOnSides;
		}
		if (ImGui::Checkbox("Selected Next Card On Use", &bSelectNextCardOnUse))
		{
			CardHand->bSelectNextCardOnUse = bSelectNextCardOnUse;
		}
		if (ImGui::Button("Add Card"))
		{
			CardHand->AddCardToHand();
		}
		if (ImGui::Button("Remove Card"))
		{
			CardHand->RemoveCardFromHand();
		}

		ImGui::Text("Cards In Hand: %i", CardHand->GetNumberOfCardsInHand());
	}

	ImGui::End();
}

void FGASC_CardHandUILayoutDebug::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}
