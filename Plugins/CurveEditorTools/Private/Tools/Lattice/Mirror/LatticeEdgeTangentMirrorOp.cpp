// Copyright Epic Games, Inc. All Rights Reserved.

#include "LatticeEdgeTangentMirrorOp.h"

#include "Tools/Lattice/LatticeDeformer2D.h"
#include "Tools/Lattice/Misc/LatticeUtils.h"
#include "MirrorUtils.h"
#include "Misc/Mirror/TangentMirrorSolver.h"
#include "Misc/Optional.h"

namespace UE::CurveEditorTools
{
namespace MirrorEdgeDetail
{
static bool CanMirror(int32 InEdgeIndex, const FLatticeDeformer2D& InDeformer)
{
	const bool bCorrectEdge = InEdgeIndex == GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Top)
		|| InEdgeIndex == GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Bottom);
	return bCorrectEdge && InDeformer.NumCells() == 1;
}
	
static TOptional<EMirroredEdgeType> GetMirroredEdgeType(int32 InEdgeIndex)
{
	constexpr int32 TopIndex = GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Top);
	constexpr int32 BottomIndex = GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Bottom);
	if (TopIndex == InEdgeIndex)
	{
		return EMirroredEdgeType::Top;
	}
	if (BottomIndex == InEdgeIndex)
	{
		return EMirroredEdgeType::Bottom;
	}
	return {};
}
	
static bool GetDragStartAndMidpointHeights(
	int32 InEdgeIndex, const FLatticeDeformer2D& InDeformer, double& OutDragStartHeight, double& OutMidPointHeight
)
{
	constexpr int32 TopIndex = GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Top);
	const bool bIsTopEdge = TopIndex == InEdgeIndex;
	const int32 OppositeEdgeIndex = bIsTopEdge ? GetEdgeIndexInSingleCellGrid(ELatticeEdgeType::Bottom) : TopIndex;

	const auto[StartVertIndex, EndVertIndex] = InDeformer.GetEdgeIndices(InEdgeIndex);
	const auto[OppositeStartVertIndex, OppositeEndVertIndex] = InDeformer.GetEdgeIndices(OppositeEdgeIndex);

	const TConstArrayView<FVector2D> ControlPoints = InDeformer.GetControlPoints();
	if (const TOptional<EMirroredEdgeType> OutEdgeType = GetMirroredEdgeType(InEdgeIndex))
	{
		OutDragStartHeight = ChooseConsistentEdgeVert(*OutEdgeType, ControlPoints[StartVertIndex], ControlPoints[EndVertIndex]);
		OutMidPointHeight = ChooseConsistentEdgeVert(*OutEdgeType, ControlPoints[OppositeStartVertIndex], ControlPoints[OppositeEndVertIndex]);
		return true;
	}
	return false;
}
}

TOptional<FLatticeEdgeTangentsMirrorOp> FLatticeEdgeTangentsMirrorOp::MakeMirrorOpForDragLatticeEdge(
	int32 InEdgeIndex, const FCurveModelID& InCurveId, const TLatticeDeformer2D<FKeyHandle>& InDeformer, const FCurveEditor& InCurveEditor
	)
{
	if (!MirrorEdgeDetail::CanMirror(InEdgeIndex, InDeformer))
	{
		return {};
	}
	
	TOptional<FCurveMirrorData> MirrorData = ComputeTangentMirrorData(InEdgeIndex, InCurveEditor, InDeformer, InCurveId, InDeformer.GetCellMetaData(0));
	if (!MirrorData)
	{
		return {};
	}
	
	return FLatticeEdgeTangentsMirrorOp(InEdgeIndex, MoveTemp(*MirrorData));
}

void FLatticeEdgeTangentsMirrorOp::OnMoveEdge(TConstArrayView<FVector2D> InNewEdge, const FCurveEditor& InCurveEditor)
{
	const TOptional<EMirroredEdgeType> EdgeType = MirrorEdgeDetail::GetMirroredEdgeType(EdgeIndex);
	if (!ensure(EdgeType.IsSet()) || !ensureMsgf(InNewEdge.Num() == 2, TEXT("We expected an edge")))
	{
		return;
	}
	
	FCurveModel* CurveModel = InCurveEditor.FindCurve(CurveMirrorData.CurveId);
	if (!CurveModel)
	{
		return;
	}
		
	CurveMirrorData.Solver.OnMoveEdge(InCurveEditor, ChooseConsistentEdgeVert(*EdgeType, InNewEdge[0], InNewEdge[1]));
}

TOptional<FLatticeEdgeTangentsMirrorOp::FCurveMirrorData> FLatticeEdgeTangentsMirrorOp::ComputeTangentMirrorData(
	int32 InEdgeIndex, const FCurveEditor& InCurveEditor,
	const FLatticeDeformer2D& InCurveDeformer, const FCurveModelID& InCurve, TConstArrayView<FKeyHandle> InKeys
	)
{
	double DragStartHeight, MidPointHeight;
	if (!MirrorEdgeDetail::GetDragStartAndMidpointHeights(InEdgeIndex, InCurveDeformer, DragStartHeight, MidPointHeight))
	{
		return {};
	}
	
	CurveEditor::FTangentMirrorSolver TangentSolver(DragStartHeight, MidPointHeight);
	if (TangentSolver.AddTangents(InCurveEditor, InCurve, InKeys))
	{
		return FCurveMirrorData(InCurve, MoveTemp(TangentSolver));
	}
	return {};
}
}
