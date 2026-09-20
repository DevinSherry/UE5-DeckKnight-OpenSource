// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Systems/Debugging/Interface/IGASCDebugPanel.h"

class UGASCourse_ShakeCharacter;

/**
 * @class FGASC_ShakeCharacterDebugPanel
 *
 * @brief Visualises every in-flight UGASCourse_ShakeCharacter ability task.
 *
 * Draws one card per active shake, tiled into as many columns as the window width allows so
 * concurrent shakes are all visible at once rather than stacked on top of each other. Each card
 * shows the actor and skeletal mesh being shaken, the ability class that started it, the live
 * per-axis offset in UE axis colours (X red, Y green, Z blue), and a graph of the authored curve
 * with a playhead.
 *
 * An instance appears here when it opted in via its bDrawDebug pin, or when
 * GASCourseDebug.ShakeCharacter.CurveGraph is enabled, which forces every instance on.
 */
class GASCOURSE_API FGASC_ShakeCharacterDebugPanel : public IIGASCDebugPanel
{
public:

	FGASC_ShakeCharacterDebugPanel();
	~FGASC_ShakeCharacterDebugPanel();

	virtual const char* GetDebugPanelName() const override { return "Shake Character"; }
	virtual void DrawDebugPanel(bool& bOpen) override;
	virtual void UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns) override;

	/** Opens the panel when shakes with debugging enabled first appear. See the base declaration. */
	virtual bool ConsumeAutoOpenRequest() override;

private:

	/** Draws one shake instance into the current ImGui child region. */
	static void DrawShakeCard(const UGASCourse_ShakeCharacter& ShakeTask, float CardWidth);

	/** Draws the three overlaid RGB curve polylines plus the playhead. */
	static void DrawCurveGraph(const UGASCourse_ShakeCharacter& ShakeTask, float GraphWidth, float GraphHeight);

	/** True while at least one live task wants to be drawn. */
	static bool HasVisibleShakes();

	/** Single source of truth for card height, used by both the cards and the window size floor. */
	static float GetCardHeight();

	/** Narrowest a card may become before the layout drops to fewer columns. */
	static float MinimumCardWidth;

	/** Height of the curve graph inside each card. */
	static float CurveGraphHeight;

	/** Previous result of HasVisibleShakes, so the auto-open request is edge-triggered. */
	bool bHadVisibleShakes = false;
};
