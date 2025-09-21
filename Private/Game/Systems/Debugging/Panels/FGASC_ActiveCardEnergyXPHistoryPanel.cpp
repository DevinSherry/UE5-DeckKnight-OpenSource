// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASC_ActiveCardEnergyXPHistoryPanel.h"
#include "Game/Systems/CardEnergy/ActiveCardEnergy/GASC_ActiveCardResourceManager.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "imgui.h"

TWeakObjectPtr<APawn> FGASC_ActiveCardEnergyXPHistoryPanel::SelectedPawn = nullptr;
TWeakObjectPtr<UGASC_ActiveCardResourceManager> FGASC_ActiveCardEnergyXPHistoryPanel::SelectedResourceManager = nullptr;
TArray<FActiveCardEnergyXPHistoryEntry> FGASC_ActiveCardEnergyXPHistoryPanel::ActiveCardEnergyXPHistoryEntries;

FGASC_ActiveCardEnergyXPHistoryPanel::FGASC_ActiveCardEnergyXPHistoryPanel()
{
}

FGASC_ActiveCardEnergyXPHistoryPanel::~FGASC_ActiveCardEnergyXPHistoryPanel()
{
}

void FGASC_ActiveCardEnergyXPHistoryPanel::DrawDebugPanel(bool& bOpen)
{
	if (ImGui::Begin("Active Card Energy XP Event History", &bOpen))
	{
		if (ImGui::BeginCombo("Pawns",
					(SelectedPawn.IsValid() ? TCHAR_TO_ANSI(*SelectedPawn->GetName()) : "None")))
		{
			for (TWeakObjectPtr<APawn> It : CachedPawns)//TActorIterator<APawn> It(GetWorld()); It; ++It
			{
				APawn* Pawn = It.Get();
				if (!IsValid(Pawn)) continue;

				bool bIsSelected = (Pawn == SelectedPawn.Get());
				FString PawnName = Pawn->GetName();

				if (ImGui::Selectable(TCHAR_TO_ANSI(*PawnName), bIsSelected))
				{
					SelectedPawn = Pawn;
					if (bIsSelected)
						ImGui::SetItemDefaultFocus();
					if (APlayerController* PC = Cast<APlayerController>(SelectedPawn->GetController()))
					{
						if (UGASC_ActiveCardResourceManager* ActiveCardResourceManager = PC->GetLocalPlayer()->GetSubsystem<UGASC_ActiveCardResourceManager>())
						{
							SelectedResourceManager = ActiveCardResourceManager;
						}
					}
				}
			}
			ImGui::EndCombo();
		}

		if (SelectedPawn.IsValid() && SelectedResourceManager.IsValid())
		{
			ActiveCardEnergyXPHistoryEntries = SelectedResourceManager->GetActiveCardEnergyHistory();
			FString TableId = FString::Printf(TEXT("Active Card Energy XP History ##Table"));
			auto TableIdANSI = StringCast<ANSICHAR>(*TableId);
			if (ImGui::BeginTable(TableIdANSI.Get(), 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
						ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
			{
				ImGui::TableSetupColumn("Event Gameplay Tag");
				ImGui::TableSetupColumn("Base Value");
				ImGui::TableSetupColumn("Modified Value");
				ImGui::TableSetupColumn("Instigator Name");
				ImGui::TableHeadersRow();
					
				for (FActiveCardEnergyXPHistoryEntry& Entry : ActiveCardEnergyXPHistoryEntries)
				{
					ImVec4 green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
					ImVec4 red   = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
						
					FString EventTagName = Entry.ActiveCardEnergyXPEventTag.GetTagName().ToString();
					ImGui::PushID(TCHAR_TO_ANSI(*EventTagName));
					ImGui::TableNextRow();
											
					ImGui::TableNextColumn();
					ImGui::Text("%s", TCHAR_TO_ANSI(*EventTagName));

					ImGui::TableNextColumn();
					ImGui::Text("%.2f", Entry.ActiveCardEnergyXPBaseValue);

					ImVec4 color = white;
					if (Entry.ActiveCardEnergyXPModifiedValue > Entry.ActiveCardEnergyXPBaseValue) color = green;
					else if (Entry.ActiveCardEnergyXPModifiedValue  < Entry.ActiveCardEnergyXPBaseValue) color = red;
											
					ImGui::TableNextColumn();
					ImGui::TextColored(color,"%.2f", Entry.ActiveCardEnergyXPModifiedValue);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", TCHAR_TO_ANSI(*Entry.ModificationToolTip));
					}

					ImGui::TableNextColumn();
					FString InstigatorName = Entry.InstigatorName;
					ImGui::Text("%s", TCHAR_TO_ANSI(*InstigatorName));
											
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}
	}
	ImGui::End();
}

void FGASC_ActiveCardEnergyXPHistoryPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}
