// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

struct FKeyHandle;

namespace UE::CurveEditorTools
{
class FLatticeDeformer2D;
template<typename TPointMetaData> class TLatticeDeformer2D;

/** We never add any points to this deformer. It's just used to keep track of the control points & edges displayed in the view. */
using FGlobalLatticeDeformer2D = FLatticeDeformer2D;
using FPerCurveDeformer2D = TLatticeDeformer2D<FKeyHandle>;
}
