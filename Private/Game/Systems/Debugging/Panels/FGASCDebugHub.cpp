// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASCDebugHub.h"
#include "imgui.h"

static bool bShowDebugHub = false;

FGASCDebugHub::FGASCDebugHub()
{
}

FGASCDebugHub::~FGASCDebugHub()
{
	DebugPanels.Empty();
}

void FGASCDebugHub::RegisterDebugPanel(const TSharedPtr<IIGASCDebugPanel> &Panel)
{
	DebugPanels.Add(Panel, false);
}

void FGASCDebugHub::DrawDebugHub()
{
	if (bShowDebugHub)
	{
		if (ImGui::Begin("Deck Knight Debug Hub", &bShowDebugHub, ImGuiWindowFlags_AlwaysAutoResize))
		{
			for (auto& PanelPair : DebugPanels)
			{
				auto Panel = PanelPair.Key;
				bool& bOpen = PanelPair.Value;

				ImGui::PushID(Panel->GetDebugPanelName());

				if (ImGui::Button(Panel->GetDebugPanelName(), { 200, 200 }))
				{
					bOpen = !bOpen;
				}
				ImGui::PopID();
				ImGui::SameLine();
			}	
		}
		ImGui::End();

		for (auto& PanelPair : DebugPanels)
		{
			auto Panel = PanelPair.Key;
			if (bool& bOpen = PanelPair.Value)
			{
				Panel->DrawDebugPanel(bOpen);
			}
		}
	}
}

void FGASCDebugHub::UpdateCachedPawns(const TArray<TWeakObjectPtr<APawn>> &Pawns)
{
	for (auto& PanelPair : DebugPanels)
	{
		auto Panel = PanelPair.Key;
		Panel->UpdateCachedPawns(Pawns);
	}
}

void FGASCDebugHub::ShowDebugHub(const bool& bInOpen)
{
	bShowDebugHub = bInOpen;
}

bool FGASCDebugHub::IsDebugHubOpen() const
{
	return bShowDebugHub;
}
