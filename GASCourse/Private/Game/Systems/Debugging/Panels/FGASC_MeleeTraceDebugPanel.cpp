#include "Game/Systems/Debugging/Panels/FGASC_MeleeTraceDebugPanel.h"
#if !UE_BUILD_SHIPPING
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Subsystem.h"
#include "GameFramework/Pawn.h"
#include "imgui.h"
#include "Game/Systems/Debugging/GASC_RewindDebugControls.h"
#include "Kismet/GameplayStatics.h"

void FGASC_MeleeTraceDebugPanel::DrawDebugPanel(bool& bOpen)
{
#if !UE_BUILD_SHIPPING
	UWorld* CurrentWorld = World.Get();
	// Game-instance panels survive travel. Prefer the current cached pawn's world when it changes.
	for (const auto& Pawn : CachedPawns)
		if (Pawn.IsValid()) { CurrentWorld = Pawn->GetWorld(); break; }
	World = CurrentWorld;
	auto* Subsystem = CurrentWorld ? CurrentWorld->GetSubsystem<UGASC_MeleeTrace_Subsystem>() : nullptr;
	if (!bPanelWasOpen) { bPanelWasOpen = true; if (CurrentWorld && !UGameplayStatics::SetGamePaused(CurrentWorld, true)) ControlError = TEXT("Game mode refused pause."); }
    if (!ImGui::Begin("Melee Traces", &bOpen)) { if (!bOpen) OnDebugPanelClosed(); ImGui::End(); return; }
	if (!Subsystem) { ImGui::TextUnformatted("No melee trace world."); ImGui::End(); return; }
	auto& Options = Subsystem->DebugOptions;
    GASC_DrawRewindWorldControls(CurrentWorld, ControlError);
    ImGui::Checkbox("Record history", &Options.bRecord); ImGui::SameLine();
    if (GASC_BeginRewindSettings())
    {
    ImGui::Checkbox("Enable debug drawing", &Options.bEnabled);
    ImGui::Checkbox("Capture characters and weapons", &Options.bCaptureActors);
    ImGui::Checkbox("Full actor ghosts", &Options.bActorGhosts); ImGui::SameLine(); ImGui::Checkbox("Actor bounds", &Options.bActorBounds);
    ImGui::SliderInt("Max visible ghost actors", &Options.MaxGhostActors, 1, 16);
    ImGui::SliderFloat("Actor capture budget / frame (ms)", &Options.ActorCaptureBudgetMs, .1f, 5.f, "%.1f");
    ImGui::Text("Actor snapshots: %.1f MiB / 64 MiB | last capture %.3f ms", double(Subsystem->ActorHistoryBytes) / (1024 * 1024), Subsystem->LastActorCaptureMs);
    ImGui::TextWrapped("Actor capture: up to 16 actors/frame, 8 meshes/actor, 512 bones/mesh, 900 frames. The renderer creates at most 2 ghost components per frame. Trace budgets remain independent.");
	ImGui::Checkbox("Shapes", &Options.bShapes);
	ImGui::SameLine();
	ImGui::Checkbox("Interpolated shapes", &Options.bInterpolated);
	ImGui::SameLine();
	ImGui::Checkbox("Sweep paths", &Options.bPaths);
	ImGui::Checkbox("Hit points", &Options.bHitPoints);
	ImGui::SameLine();
	ImGui::Checkbox("Policy / rank / group labels", &Options.bLabels);
	ImGui::SliderFloat("Draw duration (s)", &Options.DrawDuration, 0.f, 10.f);
	ImGui::Text("Draw budget: %d sweep samples (Project Settings)", GetDefault<UGASC_MeleeSubsystem_Settings>()->MaxDebugDrawSamples);
	ImGui::TextWrapped("Dense traces are sampled for display. Collision and recorded history are unaffected. Ended windows leave only a frozen trail for the draw duration.");
	ImGui::TextUnformatted("Yellow: no contact | Red: contact | HIT: accepted after policy/rank resolution");
	if (ImGui::Button("Reset to project settings"))
	{
		const auto* Settings = GetDefault<UGASC_MeleeSubsystem_Settings>();
		Options.bEnabled = Settings->bDrawDebug;
		Options.bRecord = Settings->bRecordHistory;
		Options.bShapes = Settings->bDrawShapes;
		Options.bInterpolated = Settings->bDrawInterpolatedShapes;
		Options.bPaths = Settings->bDrawSweepPaths;
		Options.bHitPoints = Settings->bDrawHitPoints;
		Options.bLabels = Settings->bDrawLabels;
		Options.DrawDuration = Settings->DebugDrawTime;
		Options.bFollowRewind = Settings->bFollowRewindDebugger;
		Options.bCaptureActors = Options.bActorGhosts = Options.bActorBounds = true; Options.MaxGhostActors = 16; Options.ActorCaptureBudgetMs = 1.f;
	}

    ImGui::EndPopup();
    }
	if (ImGui::CollapsingHeader("Characters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		TMap<TWeakObjectPtr<AActor>, FString> Actors;
		for (const auto& Pawn : CachedPawns) if (Pawn.IsValid()) Actors.Add(Pawn.Get(), Pawn->GetName());
		for (const auto& Sample : Subsystem->GetDebugHistory()) Actors.Add(Sample.Actor, Sample.ActorName);
		if (ImGui::Button("Show all")) Options.HiddenActors.Reset();
		ImGui::SameLine();
		if (ImGui::Button("Hide all")) for (const auto& Pair : Actors) Options.HiddenActors.Add(Pair.Key);
		int32 Index = 0;
		for (const auto& Pair : Actors)
		{
			ImGui::PushID(Index++);
			bool bVisible = !Options.HiddenActors.Contains(Pair.Key);
			if (ImGui::Checkbox(TCHAR_TO_UTF8(*Pair.Value), &bVisible))
			{
				if (bVisible) Options.HiddenActors.Remove(Pair.Key);
				else Options.HiddenActors.Add(Pair.Key);
			}
			ImGui::PopID();
		}
	}

	if (ImGui::CollapsingHeader("Active shapes", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("ActiveMeleeShapes", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 160)))
		{
			for (const char* Heading : {"Character / mesh", "Shape", "Rank", "Group", "Policy", "Rehit (s)", "Sockets", "Anchor"}) ImGui::TableSetupColumn(Heading);
			ImGui::TableHeadersRow();
			for (const auto& Trace : Subsystem->GetActiveTraces())
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("%s / %s", TCHAR_TO_UTF8(*GetNameSafe(Trace.InstigatorActor)), TCHAR_TO_UTF8(*GetNameSafe(Trace.SourceMeshComponent.Get())));
				ImGui::TableNextColumn(); ImGui::Text("%s [%d]", TCHAR_TO_UTF8(*Trace.ShapeName.ToString()), Trace.ShapeIndex);
				ImGui::TableNextColumn(); ImGui::Text("%d", Trace.Rank);
				ImGui::TableNextColumn(); ImGui::Text("%d", Trace.Group);
				ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(*StaticEnum<EGASC_MeleeHitPolicy>()->GetNameStringByValue(int64(Trace.HitPolicy))));
					ImGui::TableNextColumn(); ImGui::Text("%.2f", Trace.HitCooldownTime);
					ImGui::TableNextColumn(); ImGui::TextUnformatted(Trace.SocketMode == EGASC_MeleeSocketMode::SingleSocket ? "Single" : "Start / end");
					ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(*StaticEnum<EGASC_MeleeShapeAnchor>()->GetNameStringByValue(int64(Trace.ShapeAnchor))));
			}
			ImGui::EndTable();
		}
	}

	ImGui::Separator();
#if WITH_EDITOR
	ImGui::Checkbox("Follow Rewind Debugger (current PIE recording)", &Options.bFollowRewind);
	ImGui::TextWrapped("Enable before recording in Rewind Debugger. Pause PIE and scrub there to replay retained shapes. History is in memory, not stored in .utrace files.");
#endif
	if (ImGui::Checkbox("Historical playback", &Options.bPlayback) && Options.bPlayback) Options.bEnabled = true;
	ImGui::SameLine();
	if (ImGui::Button("Clear history")) { Subsystem->ClearDebugHistory(); bPlayHistory = false; }
	const auto& History = Subsystem->GetDebugHistory();
	ImGui::Text("Recorded sweep samples: %d", History.Num());
	if (Subsystem->DroppedActorFrames) ImGui::TextColored(ImVec4(1, .5f, .1f, 1), "Actor capture incomplete in %llu frames (capture limits)", static_cast<unsigned long long>(Subsystem->DroppedActorFrames));
	if (Subsystem->DroppedDebugSamples > 0)
		ImGui::TextColored(ImVec4(1, 0.5f, 0.1f, 1), "Capture truncated: %llu samples over the memory budget. Tracing was unaffected.",
			static_cast<unsigned long long>(Subsystem->DroppedDebugSamples));
	if (History.IsEmpty())
		ImGui::TextUnformatted("Enable Record history before attacking.");
	else
	{
		const double MinTime = History[0].FrameStartTime;
		const double MaxTime = History.Last().Time;
		const ImVec2 TimelineOrigin = ImGui::GetCursorScreenPos();
		const ImVec2 TimelineSize(FMath::Max(100.f, ImGui::GetContentRegionAvail().x), 32.f);
		ImGui::InvisibleButton("Hit timeline", TimelineSize);
		auto* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRectFilled(TimelineOrigin, ImVec2(TimelineOrigin.x + TimelineSize.x, TimelineOrigin.y + TimelineSize.y), IM_COL32(25, 25, 30, 255));
		TMap<double, bool> FrameHits;
		for (const auto& Sample : History)
			if (!Options.HiddenActors.Contains(Sample.Actor)) FrameHits.FindOrAdd(Sample.Time) |= Sample.bAccepted;
		for (const auto& Pair : FrameHits)
		{
			const float X = TimelineOrigin.x + float((Pair.Key - MinTime) / FMath::Max(0.001, MaxTime - MinTime)) * TimelineSize.x;
			DrawList->AddLine(ImVec2(X, TimelineOrigin.y + 3), ImVec2(X, TimelineOrigin.y + TimelineSize.y - 3),
				Pair.Value ? IM_COL32(255, 50, 50, 255) : IM_COL32(240, 210, 40, 255), 2.f);
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDown(0))
		{
			Options.PlaybackTime = FMath::Lerp(MinTime, MaxTime, double(FMath::Clamp((ImGui::GetIO().MousePos.x - TimelineOrigin.x) / TimelineSize.x, 0.f, 1.f)));
			Options.bPlayback = true; Options.bEnabled = true; Options.bFollowRewind = false; bPlayHistory = false;
		}
		Options.PlaybackTime = FMath::Clamp(Options.PlaybackTime, MinTime, MaxTime);
		if (ImGui::SliderScalar("Timeline (world seconds)", ImGuiDataType_Double, &Options.PlaybackTime, &MinTime, &MaxTime, "%.3f s"))
		{
			Options.bPlayback = true; Options.bEnabled = true;
			Options.bFollowRewind = false;
			bPlayHistory = false;
		}
		ImGui::Dummy(ImVec2(0, 12));
		if (ImGui::Button(bPlayHistory ? "Pause timeline" : "Play timeline"))
		{
			bPlayHistory = !bPlayHistory;
			Options.bPlayback = true; Options.bEnabled = true;
			Options.bFollowRewind = false;
			if (Options.PlaybackTime >= MaxTime) Options.PlaybackTime = MinTime;
		}
		ImGui::SameLine();
		if (ImGui::Button("Previous frame"))
		{
			for (int32 Index = History.Num() - 1; Index >= 0; --Index)
				if (History[Index].Time < Options.PlaybackTime - 0.00001) { Options.PlaybackTime = History[Index].Time; break; }
			Options.bPlayback = true; Options.bEnabled = true; Options.bFollowRewind = false; bPlayHistory = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Next frame"))
		{
			for (const auto& Sample : History)
				if (Sample.Time > Options.PlaybackTime + 0.00001) { Options.PlaybackTime = Sample.Time; break; }
			Options.bPlayback = true; Options.bEnabled = true; Options.bFollowRewind = false; bPlayHistory = false;
		}
		ImGui::SliderFloat("Playback speed", &PlaybackSpeed, 0.1f, 2.f);
		if (bPlayHistory && Options.bPlayback && !Options.bFollowRewind)
		{
			Options.PlaybackTime = FMath::Min(MaxTime, Options.PlaybackTime + ImGui::GetIO().DeltaTime * PlaybackSpeed);
			if (Options.PlaybackTime >= MaxTime) bPlayHistory = false;
		}

		const auto* Selected = History.FindByPredicate([&Options](const auto& Sample)
			{ return Sample.FrameStartTime <= Options.PlaybackTime && Options.PlaybackTime <= Sample.Time; });
		if (Selected)
		{
			int32 Contacts = 0, Accepted = 0, Limited = 0;
			for (const auto& Sample : History)
				if (Sample.Frame == Selected->Frame && !Options.HiddenActors.Contains(Sample.Actor))
				{
					Contacts += Sample.bHit;
					Accepted += Sample.bAccepted;
					Limited += Sample.bBudgetLimited;
				}
			ImGui::Text("Contacts: %d | Accepted hits: %d | Samples at step budget: %d", Contacts, Accepted, Limited);
		}
	}
	if (!bOpen) OnDebugPanelClosed();
	ImGui::End();
#endif
}

void FGASC_MeleeTraceDebugPanel::OnDebugPanelClosed()
{
 bPanelWasOpen = false; bPlayHistory = false;
 if (World.IsValid()) if (auto* Subsystem = World->GetSubsystem<UGASC_MeleeTrace_Subsystem>()) { Subsystem->DebugOptions.bPlayback = false; Subsystem->ClearDebugGhosts(); }
}
#endif // !UE_BUILD_SHIPPING
