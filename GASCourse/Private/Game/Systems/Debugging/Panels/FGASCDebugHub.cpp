#include "Game/Systems/Debugging/Panels/FGASCDebugHub.h"
#if !UE_BUILD_SHIPPING
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
			const int NumColumns = 4;
			const ImVec2 ButtonSize(200, 200);

			int ColumnIndex = 0;

			for (auto& PanelPair : DebugPanels)
			{
				auto Panel = PanelPair.Key;
				bool& bOpen = PanelPair.Value;

				ImGui::PushID(Panel->GetDebugPanelName());

				if (ImGui::Button(Panel->GetDebugPanelName(), ButtonSize))
				{
					bOpen = !bOpen;
					if (!bOpen) Panel->OnDebugPanelClosed();
				}
				ImGui::PopID();

				ColumnIndex++;

				// Move to next row every 4 buttons
				if (ColumnIndex % NumColumns != 0)
				{
					ImGui::SameLine();
				}
			}
		}
		ImGui::End();
	}

	// Panels are drawn outside the hub check on purpose. The hub is a launcher, not a container:
	// a panel that is open - or that asks to open itself because something in the game turned its
	// debugging on - has to keep drawing whether or not the launcher window happens to be visible.
	// Panels that are closed and do not request auto-open still never draw.
	for (auto& PanelPair : DebugPanels)
	{
		auto Panel = PanelPair.Key;
		bool& bOpen = PanelPair.Value;

		if (!bOpen && Panel->ConsumeAutoOpenRequest())
		{
			bOpen = true;
		}

		if (bOpen)
		{
			Panel->DrawDebugPanel(bOpen);
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

#else
FGASCDebugHub::FGASCDebugHub() = default;
FGASCDebugHub::~FGASCDebugHub() = default;
void FGASCDebugHub::RegisterDebugPanel(const TSharedPtr<IIGASCDebugPanel>&) {}
void FGASCDebugHub::DrawDebugHub() {}
void FGASCDebugHub::UpdateCachedPawns(const TArray<TWeakObjectPtr<APawn>>&) {}
void FGASCDebugHub::ShowDebugHub(const bool&) {}
bool FGASCDebugHub::IsDebugHubOpen() const { return false; }
#endif
