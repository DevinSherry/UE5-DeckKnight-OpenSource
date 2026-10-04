// Copyright Epic Games, Inc. All Rights Reserved.

#include "LatticeUndoObject.h"

#include "CurveEditor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LatticeUndoObject)

void UCurveEditorTools_LatticeUndoObject::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Snapshot;
}

void UCurveEditorTools_LatticeUndoObject::PostEditUndo()
{
	UObject::PostEditUndo();
	OnPostEditUndo.Broadcast();
}
