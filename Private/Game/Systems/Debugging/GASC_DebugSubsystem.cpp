// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Debugging/GASC_DebugSubsystem.h"
#include "imgui.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Game/Systems/Debugging/Panels/FGASCAttributesPanel.h"
#include "Game/Systems/Debugging/Panels/FGASC_ActiveCardEnergyXPHistoryPanel.h"
#include "Game/Systems/Debugging/Panels/FGASC_WaveManagerPanel.h"
#include "Game/Systems/Debugging/Panels/GASCDamageEventsPanel.h"

void UGASC_DebugSubsystem::Tick(float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		CacheAllPawns(World);
		DebugHub.UpdateCachedPawns(CachedPawns);
		DrawImGui();
	}
}

TStatId UGASC_DebugSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGASC_DebugSubsystem, STATGROUP_Tickables);
}

void UGASC_DebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DebugHub.RegisterDebugPanel(MakeShared<FGASCAttributesPanel>());
	DebugHub.RegisterDebugPanel(MakeShared<FGASC_ActiveCardEnergyXPHistoryPanel>());
	DebugHub.RegisterDebugPanel(MakeShared<FGASC_WaveManagerPanel>());
	DebugHub.RegisterDebugPanel(MakeShared<FGASCDamageEventsPanel>());
}

void UGASC_DebugSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UGASC_DebugSubsystem::ToggleGameplayDebugHUD(bool bInOpen)
{
	DebugHub.ShowDebugHub(bInOpen);
}

bool UGASC_DebugSubsystem::IsDebugOpen() const
{
	return DebugHub.IsDebugHubOpen();
}

void UGASC_DebugSubsystem::DrawImGui()
{
	if (!ImGui::GetCurrentContext())
	{
		return;
	}
	DebugHub.DrawDebugHub();
}

void UGASC_DebugSubsystem::CacheAllPawns(UWorld* World)
{
	CachedPawns.Empty();

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (IsValid(Pawn))
		{
			CachedPawns.Add(Pawn);
		}
	}

	// Clean up history for destroyed actors
	TArray<AActor*> ActorsToRemove;
	for (auto& Pair : AttributeHistory)
	{
		if (!IsValid(Pair.Key))
		{
			ActorsToRemove.Add(Pair.Key);
		}
	}
    
	for (AActor* Actor : ActorsToRemove)
	{
		AttributeHistory.Remove(Actor);
	}
}