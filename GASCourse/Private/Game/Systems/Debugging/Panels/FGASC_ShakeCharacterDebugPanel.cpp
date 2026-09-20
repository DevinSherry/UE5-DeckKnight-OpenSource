#include "Game/Systems/Debugging/Panels/FGASC_ShakeCharacterDebugPanel.h"
#if !UE_BUILD_SHIPPING
// Fill out your copyright notice in the Description page of Project Settings.



#include "imgui.h"
#include "Game/GameplayAbilitySystem/Tasks/Gameplay/GASCourse_ShakeCharacter.h"

#if !UE_BUILD_SHIPPING

namespace GASC_ShakeDebugPanelConstants
{
	/** UE axis convention: X red, Y green, Z blue. */
	static const ImVec4 AxisColourX(1.0f, 0.25f, 0.25f, 1.0f);
	static const ImVec4 AxisColourY(0.35f, 1.0f, 0.35f, 1.0f);
	static const ImVec4 AxisColourZ(0.4f, 0.55f, 1.0f, 1.0f);

	static const ImVec4 MutedTextColour(0.65f, 0.65f, 0.65f, 1.0f);

	/** Padding between tiled cards. */
	static constexpr float CardSpacing = 8.0f;
}

#endif

// Defined unconditionally: the header declares them unconditionally, so keeping the definitions
// outside the shipping guard avoids a declared-but-undefined static if they are ever referenced.
float FGASC_ShakeCharacterDebugPanel::MinimumCardWidth = 320.0f;
float FGASC_ShakeCharacterDebugPanel::CurveGraphHeight = 110.0f;

FGASC_ShakeCharacterDebugPanel::FGASC_ShakeCharacterDebugPanel()
{
}

FGASC_ShakeCharacterDebugPanel::~FGASC_ShakeCharacterDebugPanel()
{
}

void FGASC_ShakeCharacterDebugPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}

bool FGASC_ShakeCharacterDebugPanel::HasVisibleShakes()
{
#if !UE_BUILD_SHIPPING
	for (const TWeakObjectPtr<UGASCourse_ShakeCharacter>& WeakShake : UGASCourse_ShakeCharacter::GetActiveShakeTasksForDebug())
	{
		const UGASCourse_ShakeCharacter* ShakeTask = WeakShake.Get();
		if (ShakeTask && ShakeTask->ShouldDrawDebug())
		{
			return true;
		}
	}
#endif

	return false;
}

float FGASC_ShakeCharacterDebugPanel::GetCardHeight()
{
	// Graph height plus room for the seven text lines above it: actor, mesh, source, mode, loop
	// state, amplitude deviation and the axis readout. Bump the constant if more lines are added.
	return CurveGraphHeight + 132.0f;
}

bool FGASC_ShakeCharacterDebugPanel::ConsumeAutoOpenRequest()
{
	const bool bHasVisibleShakes = HasVisibleShakes();

	// Edge-triggered: only request on the transition into having something to show. Returning true
	// continuously would make the panel impossible to dismiss while a long shake is running.
	const bool bShouldRequestOpen = bHasVisibleShakes && !bHadVisibleShakes;
	bHadVisibleShakes = bHasVisibleShakes;

	return bShouldRequestOpen;
}

void FGASC_ShakeCharacterDebugPanel::DrawDebugPanel(bool& bOpen)
{
#if !UE_BUILD_SHIPPING
	// Vertical room the chrome above the cards needs: the two sliders, a separator, and the active
	// count line.
	constexpr float HeaderAllowance = 96.0f;
	constexpr float WindowPadding = 24.0f;

	const ImVec2 MinimumWindowSize(
		MinimumCardWidth + GASC_ShakeDebugPanelConstants::CardSpacing + WindowPadding,
		GetCardHeight() + HeaderAllowance + WindowPadding);

	// A hard floor, so the window can never be dragged smaller than one fully visible card and
	// curve. This is what removes the need to resize it by hand to see anything.
	ImGui::SetNextWindowSizeConstraints(MinimumWindowSize, ImVec2(FLT_MAX, FLT_MAX));

	// Roomy first-run size - about two columns - without overriding a size chosen since.
	// Deliberately not ImGuiWindowFlags_AlwaysAutoResize: the tiling below reads
	// GetContentRegionAvail().x to pick a column count, and under auto-resize that width is itself
	// derived from the content, which is circular and makes the layout oscillate.
	ImGui::SetNextWindowSize(ImVec2(MinimumWindowSize.x * 2.0f, MinimumWindowSize.y), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Shake Character", &bOpen, ImGuiWindowFlags_HorizontalScrollbar))
	{
		ImGui::End();
		return;
	}

	// Collect first so the column count is known before anything is drawn.
	TArray<const UGASCourse_ShakeCharacter*> VisibleShakes;
	for (const TWeakObjectPtr<UGASCourse_ShakeCharacter>& WeakShake : UGASCourse_ShakeCharacter::GetActiveShakeTasksForDebug())
	{
		const UGASCourse_ShakeCharacter* ShakeTask = WeakShake.Get();
		if (ShakeTask && ShakeTask->ShouldDrawDebug())
		{
			VisibleShakes.Add(ShakeTask);
		}
	}

	ImGui::SliderFloat("Min Card Width", &MinimumCardWidth, 200.0f, 600.0f);
	ImGui::SliderFloat("Graph Height", &CurveGraphHeight, 60.0f, 240.0f);
	ImGui::Separator();

	if (VisibleShakes.Num() == 0)
	{
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour,
			"No active shakes with debug enabled.");
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour,
			"Tick bDrawDebug on the task, or set GASCourseDebug.ShakeCharacter.CurveGraph 1");
		ImGui::End();
		return;
	}

	ImGui::Text("Active shakes: %d", VisibleShakes.Num());
	ImGui::Separator();

	// Tile into as many columns as fit, so simultaneous shakes sit side by side instead of
	// overlapping. Recomputed every frame, so it reflows as the window is resized.
	const float AvailableWidth = ImGui::GetContentRegionAvail().x;
	const int32 ColumnCount = FMath::Max(1, FMath::FloorToInt(AvailableWidth / MinimumCardWidth));
	const float CardWidth = FMath::Max(
		MinimumCardWidth * 0.5f,
		(AvailableWidth / static_cast<float>(ColumnCount)) - GASC_ShakeDebugPanelConstants::CardSpacing);

	for (int32 ShakeIndex = 0; ShakeIndex < VisibleShakes.Num(); ++ShakeIndex)
	{
		const UGASCourse_ShakeCharacter* ShakeTask = VisibleShakes[ShakeIndex];

		ImGui::PushID(ShakeIndex);
		if (ImGui::BeginChild("ShakeCard", ImVec2(CardWidth, GetCardHeight()), ImGuiChildFlags_Borders))
		{
			DrawShakeCard(*ShakeTask, CardWidth);
		}
		ImGui::EndChild();
		ImGui::PopID();

		// Keep filling the current row until the column count is reached.
		const bool bIsLastInRow = ((ShakeIndex + 1) % ColumnCount) == 0;
		const bool bIsLastOverall = ShakeIndex == (VisibleShakes.Num() - 1);
		if (!bIsLastInRow && !bIsLastOverall)
		{
			ImGui::SameLine();
		}
	}

	ImGui::End();
#endif
}

void FGASC_ShakeCharacterDebugPanel::DrawShakeCard(const UGASCourse_ShakeCharacter& ShakeTask, float CardWidth)
{
#if !UE_BUILD_SHIPPING
	const FString ActorName = ShakeTask.GetDebugTargetActorName();
	const FString MeshName = ShakeTask.GetDebugMeshComponentName();
	const FString InvokerName = ShakeTask.GetDebugInvokingClassName();
	const FString InvokerPath = ShakeTask.GetDebugInvokingClassPath();

	ImGui::Text("Actor: %s", TCHAR_TO_ANSI(*ActorName));
	ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour, "Mesh: %s", TCHAR_TO_ANSI(*MeshName));

	ImGui::Text("From: %s", TCHAR_TO_ANSI(*InvokerName));
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", TCHAR_TO_ANSI(*InvokerPath));
	}

	// Mode line doubles as an explanation of why the axis mask is or is not relevant.
	if (ShakeTask.IsVectorCurveMode())
	{
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour, "Mode: Vector curve (all axes)");
	}
	else
	{
		const int32 Mask = ShakeTask.GetShakeAxesMask();
		const EGASCourseShakeTaskAxes Axes = static_cast<EGASCourseShakeTaskAxes>(Mask);
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour, "Mode: Float curve, axes %s%s%s",
			EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::X) ? "X" : "-",
			EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::Y) ? "Y" : "-",
			EnumHasAnyFlags(Axes, EGASCourseShakeTaskAxes::Z) ? "Z" : "-");
	}

	// Playback mode and whether it ever ends on its own - the combination of loop, ping-pong and a
	// duration override is not obvious from the pins alone.
	const float TotalLifetime = ShakeTask.GetDebugTotalLifetime();
	if (TotalLifetime > 0.0f)
	{
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour, "Play: %s%s, ends at %.2fs",
			ShakeTask.IsLooping() ? "loop" : "once",
			ShakeTask.IsPingPong() ? " ping-pong" : "",
			TotalLifetime);
	}
	else
	{
		ImGui::TextColored(GASC_ShakeDebugPanelConstants::MutedTextColour, "Play: loop%s, indefinite",
			ShakeTask.IsPingPong() ? " ping-pong" : "");
	}

	ImGui::Text("Amplitude deviation: %+.2f%%", ShakeTask.GetDebugCurrentAmplitudeDeviation() * 100.0f);

	// Live offset, in UE axis colours.
	const FVector CurrentOffset = ShakeTask.GetDebugCurrentOffset();
	ImGui::TextColored(GASC_ShakeDebugPanelConstants::AxisColourX, "X %+.2f", CurrentOffset.X);
	ImGui::SameLine();
	ImGui::TextColored(GASC_ShakeDebugPanelConstants::AxisColourY, "Y %+.2f", CurrentOffset.Y);
	ImGui::SameLine();
	ImGui::TextColored(GASC_ShakeDebugPanelConstants::AxisColourZ, "Z %+.2f", CurrentOffset.Z);

	DrawCurveGraph(ShakeTask, CardWidth - 20.0f, CurveGraphHeight);
#endif
}

void FGASC_ShakeCharacterDebugPanel::DrawCurveGraph(const UGASCourse_ShakeCharacter& ShakeTask, float GraphWidth, float GraphHeight)
{
#if !UE_BUILD_SHIPPING
	const TArray<FVector>& Samples = ShakeTask.GetDebugCurveSamples();

	// Reserve the rect with a Dummy so ImGui lays out correctly, then paint into it. PlotLines
	// would only give one series per call, so it cannot overlay three RGB curves in one graph.
	const ImVec2 GraphOrigin = ImGui::GetCursorScreenPos();
	const ImVec2 GraphSize(FMath::Max(GraphWidth, 40.0f), FMath::Max(GraphHeight, 40.0f));
	ImGui::Dummy(GraphSize);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	if (!DrawList)
	{
		return;
	}

	const ImVec2 GraphMax(GraphOrigin.x + GraphSize.x, GraphOrigin.y + GraphSize.y);
	DrawList->AddRectFilled(GraphOrigin, GraphMax, IM_COL32(18, 18, 22, 255));
	DrawList->AddRect(GraphOrigin, GraphMax, IM_COL32(80, 80, 90, 255));

	if (Samples.Num() < 2)
	{
		return;
	}

	// One shared vertical scale across all three axes, so their relative magnitudes read honestly.
	const FVector Peak = ShakeTask.GetDebugPeakOffset();
	const float MaxMagnitude = FMath::Max3(Peak.X, Peak.Y, Peak.Z);
	if (MaxMagnitude <= KINDA_SMALL_NUMBER)
	{
		DrawList->AddText(ImVec2(GraphOrigin.x + 6.0f, GraphOrigin.y + 6.0f), IM_COL32(150, 150, 150, 255), "Curve is flat at zero");
		return;
	}

	const float MidY = GraphOrigin.y + (GraphSize.y * 0.5f);
	const float HalfHeight = (GraphSize.y * 0.5f) - 4.0f;

	// Zero line.
	DrawList->AddLine(ImVec2(GraphOrigin.x, MidY), ImVec2(GraphMax.x, MidY), IM_COL32(70, 70, 80, 255));

	const auto PlotAxis = [&](TFunctionRef<float(const FVector&)> AxisGetter, ImU32 LineColour)
	{
		for (int32 SampleIndex = 0; SampleIndex < Samples.Num() - 1; ++SampleIndex)
		{
			const float AlphaA = static_cast<float>(SampleIndex) / static_cast<float>(Samples.Num() - 1);
			const float AlphaB = static_cast<float>(SampleIndex + 1) / static_cast<float>(Samples.Num() - 1);

			const ImVec2 PointA(
				GraphOrigin.x + AlphaA * GraphSize.x,
				MidY - (AxisGetter(Samples[SampleIndex]) / MaxMagnitude) * HalfHeight);
			const ImVec2 PointB(
				GraphOrigin.x + AlphaB * GraphSize.x,
				MidY - (AxisGetter(Samples[SampleIndex + 1]) / MaxMagnitude) * HalfHeight);

			DrawList->AddLine(PointA, PointB, LineColour, 1.5f);
		}
	};

	PlotAxis([](const FVector& Sample) { return static_cast<float>(Sample.X); }, IM_COL32(255, 64, 64, 255));
	PlotAxis([](const FVector& Sample) { return static_cast<float>(Sample.Y); }, IM_COL32(90, 255, 90, 255));
	PlotAxis([](const FVector& Sample) { return static_cast<float>(Sample.Z); }, IM_COL32(100, 140, 255, 255));

	// Playhead.
	const float PlayheadX = GraphOrigin.x + ShakeTask.GetDebugPlayheadAlpha() * GraphSize.x;
	DrawList->AddLine(ImVec2(PlayheadX, GraphOrigin.y), ImVec2(PlayheadX, GraphMax.y), IM_COL32(255, 255, 255, 200), 1.0f);

	// Peak magnitude, so the vertical scale is legible.
	const FString ScaleLabel = FString::Printf(TEXT("peak %.2f"), MaxMagnitude);
	DrawList->AddText(ImVec2(GraphOrigin.x + 6.0f, GraphOrigin.y + 4.0f), IM_COL32(140, 140, 150, 255), TCHAR_TO_ANSI(*ScaleLabel));
#endif
}

#endif // !UE_BUILD_SHIPPING
