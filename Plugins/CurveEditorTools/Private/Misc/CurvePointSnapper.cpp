// Copyright Epic Games, Inc. All Rights Reserved.

#include "CurvePointSnapper.h"

#include "CurveEditor.h"
#include "CurveEditorTypes.h"
#include "SCurveEditorView.h"
#include "Misc/Optional.h"

namespace UE::CurveEditorTools
{
TOptional<FCurvePointSnapper> FCurvePointSnapper::MakeSnapper(FCurveEditor& InCurveEditor)
{
	if (InCurveEditor.GetSelection().GetAll().IsEmpty())
	{
		return {};
	}
	
	// Snap the mouse to the grid according to the first curve's snap metrics (assuming all curves have the same view scales)
	const FCurveModelID FirstCurveID = InCurveEditor.GetSelection().GetAll().CreateConstIterator()->Key;
	const SCurveEditorView* View = InCurveEditor.FindFirstInteractiveView(FirstCurveID);
	if (!View)
	{
		return {};
	}

	return FCurvePointSnapper(View->GetCurveSpace(FirstCurveID), InCurveEditor.GetCurveSnapMetrics(FirstCurveID));
}

FVector2D FCurvePointSnapper::SnapPoint(const FVector2D& InCurveSpacePoint) const
{
	return FVector2D
	{
		SnapMetrics.SnapInputSeconds(InCurveSpacePoint.X),
		SnapMetrics.SnapOutput(InCurveSpacePoint.Y)
	};
}

FKeyPosition FCurvePointSnapper::SnapKey(const FKeyPosition& InKeyPosition) const
{
	const FVector2D Point = SnapPoint({ InKeyPosition.InputValue, InKeyPosition.OutputValue });
	return { Point.X, Point.Y };
}
}
