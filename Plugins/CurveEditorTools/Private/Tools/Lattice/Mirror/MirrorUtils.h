// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"

namespace UE::CurveEditorTools
{
enum class EMirroredEdgeType { Top, Bottom };

/** Consistenly chooses the same vertex on the top or bottom edge ASSUMING that the vertices are always transformed uniformly (e.g. move both up by x units, etc.) */
inline double ChooseConsistentEdgeVert(EMirroredEdgeType EdgeType, const FVector2D& Vert1, const FVector2D& Vert2)
{
	// In several places, we need to consistently choose vert 1 or vert 2 to take the height from.
	// During the edge drag operation the vertices are moved up and down by the same amounts.
	// For top edge we choose the higher, and for bottom edge the lower vertex on the edge, which guarantees we consistently choose the same vertex.
	// Choosing top and bottom vertex, respectively, is done so with non-parallel edges the tangents are interpolated as slowly as possible, i.e.
	// you need to move the mouse more. 
	switch (EdgeType)
	{
	case EMirroredEdgeType::Top: return FMath::Max(Vert1.Y, Vert2.Y);
	case EMirroredEdgeType::Bottom: return FMath::Min(Vert1.Y, Vert2.Y);
	default: checkNoEntry(); return 0;
	}
}
}
