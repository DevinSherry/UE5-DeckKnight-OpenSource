#include "Debugging/GASC_MeleeRewindExtension.h"
#include "IRewindDebugger.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Subsystem.h"
#include "UObject/UObjectIterator.h"

void FGASC_MeleeRewindExtension::RecordingStarted(IRewindDebugger* Debugger)
{
	Clear(Debugger);
	for (TObjectIterator<UGASC_MeleeTrace_Subsystem> It; It; ++It)
	{
		UWorld* World = It->GetWorld();
		if (!It->IsInitialized() || !World || World->WorldType != EWorldType::PIE || !It->DebugOptions.bFollowRewind) continue;
		Captures.Add({*It, It->DebugOptions.bRecord});
		It->ClearDebugHistory();
		It->DebugOptions.bRecord = true;
	}
}

void FGASC_MeleeRewindExtension::RecordingStopped(IRewindDebugger* Debugger)
{
	for (const auto& Capture : Captures)
		if (auto* Subsystem = Capture.Subsystem.Get())
			Subsystem->DebugOptions.bRecord = Capture.bWasRecording;
}

void FGASC_MeleeRewindExtension::Update(float DeltaTime, IRewindDebugger* Debugger)
{
	if (!Debugger) return;
	if (Debugger->IsTraceFileLoaded())
	{
		for (const auto& Capture : Captures) if (auto* Subsystem = Capture.Subsystem.Get()) Subsystem->ClearDebugGhosts();
		return;
	}
	for (const auto& Capture : Captures)
	{
		auto* Subsystem = Capture.Subsystem.Get();
		if (!Subsystem) continue;
		if (!Subsystem->DebugOptions.bFollowRewind || !Subsystem->DebugOptions.bEnabled) { Subsystem->ClearDebugGhosts(); continue; }
		if (Debugger->IsPIESimulating())
		{
			Subsystem->DebugOptions.bPlayback = false;
			Subsystem->ClearDebugGhosts();
			continue;
		}
		UWorld* World = Debugger->GetWorldToVisualize();
		// Histories are world-specific: do not overlay another PIE client's/server's shapes.
		if (!World || World != Subsystem->GetWorld()) { Subsystem->ClearDebugGhosts(); continue; }
		Subsystem->DebugOptions.bPlayback = true;
		const double ScrubTime = Debugger->GetScrubTime();
		for (const auto& Sample : Subsystem->GetDebugHistory())
			if (Sample.RecordingFrameStartTime <= ScrubTime && ScrubTime <= Sample.RecordingTime)
			{
				Subsystem->DebugOptions.PlaybackTime = Sample.Time;
				break;
			}
		if (Subsystem->DebugOptions.bEnabled)
			Subsystem->DrawDebugHistory(World, ScrubTime, true);
	}
}

void FGASC_MeleeRewindExtension::Clear(IRewindDebugger* Debugger)
{
	for (const auto& Capture : Captures)
		if (auto* Subsystem = Capture.Subsystem.Get())
		{
			Subsystem->DebugOptions.bRecord = Capture.bWasRecording;
			Subsystem->DebugOptions.bPlayback = false;
			Subsystem->ClearDebugGhosts();
		}
	Captures.Reset();
}
