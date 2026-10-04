// Copyright Epic Games, Inc. All Rights Reserved.

#include "LatticeDragOp.h"

#include "CurveEditor.h"

namespace UE::CurveEditorTools
{
FLatticeDragOp::FLatticeDragOp(TWeakPtr<FCurveEditor> InCurveEditor)
	: CurveEditor(MoveTemp(InCurveEditor))
	, InitialMousePosition()
{}

void FLatticeDragOp::BeginDrag(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, const FVector2D& InInitialMousePosition)
{
	InitialMousePosition = LastMousePosition = InMouseEvent.GetScreenSpacePosition();
	AccumulateMouseMovement(InGeometry, InMouseEvent);
	OnBeginDrag(InGeometry, InitialMousePosition);
}

void FLatticeDragOp::MoveMouse(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	AccumulateMouseMovement(InGeometry, InMouseEvent);
}

void FLatticeDragOp::FinishedPointerInput()
{
	if (AccumulatedMouseMovement)
	{
		OnMoveMouse(AccumulatedMouseMovement->CachedGeometry, AccumulatedMouseMovement->AccumulatedPosition);
		AccumulatedMouseMovement.Reset();
	}
}

void FLatticeDragOp::EndDrag(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Snap the mouse position according to the axis snapping settings.
	AccumulateMouseMovement(InGeometry, InMouseEvent);
	OnEndDrag(InGeometry, AccumulatedMouseMovement->AccumulatedPosition);
	AccumulatedMouseMovement.Reset();
}

void FLatticeDragOp::CancelDrag()
{
	OnCancelDrag();
}

void FLatticeDragOp::AccumulateMouseMovement(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const TSharedPtr<FCurveEditor> CurveEditorPin = CurveEditor.Pin();
	if (!CurveEditorPin)
	{
		return;
	}
	
	const FVector2D ScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();
	const FVector2D SnappedPosition = CurveEditorPin->GetAxisSnap().GetSnappedPosition(
		InitialMousePosition, ScreenSpacePosition, LastMousePosition, InMouseEvent, SnapState
		);
	LastMousePosition = ScreenSpacePosition;
	
	if (!AccumulatedMouseMovement)
	{
		AccumulatedMouseMovement.Emplace(InGeometry, SnappedPosition);
	}
	else
	{
		AccumulatedMouseMovement->AccumulatedPosition = SnappedPosition;
	}
}
}
