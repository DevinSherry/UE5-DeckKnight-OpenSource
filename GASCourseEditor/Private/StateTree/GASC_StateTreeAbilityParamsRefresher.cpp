// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTree/GASC_StateTreeAbilityParamsRefresher.h"

#include "Editor.h"
#include "StateTree.h"
#include "StateTreeDelegates.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorNode.h"
#include "StateTreeState.h"
#include "Game/StateTree/Tasks/STT_GrantAbilityDynamic.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(LOG_GASC_StateTreeAbilityParams);

namespace
{
	/**
	 * Re-syncs one editor node if it holds a Grant Ability Dynamic task.
	 *
	 * GetMutablePtr returns null for every other kind of node, which is what makes this safe to point
	 * at any node array, and it accepts subclasses of the task as well.
	 *
	 * @return true if the node's parameter bag changed.
	 */
	bool SyncEditorNode(FStateTreeEditorNode& EditorNode)
	{
		FGrantAbilityDynamicTask* Task = EditorNode.Node.GetMutablePtr<FGrantAbilityDynamicTask>();
		if (!Task)
		{
			return false;
		}

		return Task->ConditionallySyncAbilityParams(EditorNode.GetInstance());
	}

	/** @return the number of nodes changed across the array. */
	int32 SyncEditorNodes(TArray<FStateTreeEditorNode>& EditorNodes)
	{
		int32 NumChanged = 0;
		for (FStateTreeEditorNode& EditorNode : EditorNodes)
		{
			NumChanged += SyncEditorNode(EditorNode) ? 1 : 0;
		}
		return NumChanged;
	}
}

void UGASC_StateTreeAbilityParamsRefresher::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEditor)
	{
		// OnBlueprintCompiled carries no arguments and fires once per compile batch rather than once
		// per Blueprint, so there is nothing to filter on here. Pairing it with OnBlueprintPreCompile
		// to record whether an ability Blueprint was among the batch was considered and rejected: the
		// two are not broadcast from all the same code paths, so a missed pre-compile would silently
		// turn into a missed refresh, which is the exact failure this subsystem exists to remove. The
		// unconditional sweep is cheap instead - see RefreshLoadedStateTrees.
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddUObject(
			this, &UGASC_StateTreeAbilityParamsRefresher::HandleBlueprintCompiled);
	}
}

void UGASC_StateTreeAbilityParamsRefresher::Deinitialize()
{
	if (GEditor && BlueprintCompiledHandle.IsValid())
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
	}
	BlueprintCompiledHandle.Reset();

	Super::Deinitialize();
}

void UGASC_StateTreeAbilityParamsRefresher::HandleBlueprintCompiled()
{
	RefreshLoadedStateTrees();
}

int32 UGASC_StateTreeAbilityParamsRefresher::RefreshStateTree(UStateTree& StateTree)
{
	// A cooked or compiled-only asset has no editor data, and so no authored nodes to re-sync.
	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree.EditorData);
	if (!EditorData)
	{
		return 0;
	}

	int32 NumChanged = 0;

	// Global tasks live on the editor data itself rather than on any state.
	if (const int32 NumGlobalChanged = SyncEditorNodes(EditorData->GlobalTasks); NumGlobalChanged > 0)
	{
		EditorData->Modify(/*bAlwaysMarkDirty=*/false);
		NumChanged += NumGlobalChanged;
	}

	// VisitHierarchy walks the whole tree from its subtree roots and hands back each state mutably,
	// which is why it works from an asset nobody has opened an editor for.
	EditorData->VisitHierarchy([&NumChanged](UStateTreeState& State, UStateTreeState* /*ParentState*/)
	{
		// A state holds its tasks either in the list or, when it runs a single one, in SingleTask.
		// Both are populated independently of State.Type, so both are always worth checking.
		int32 NumStateChanged = SyncEditorNodes(State.Tasks);
		NumStateChanged += SyncEditorNode(State.SingleTask) ? 1 : 0;

		if (NumStateChanged > 0)
		{
			State.Modify(/*bAlwaysMarkDirty=*/false);
			NumChanged += NumStateChanged;
		}

		return EStateTreeVisitor::Continue;
	});

	if (NumChanged > 0)
	{
		// The details panel caches the set of bindable structs, so a rebuilt bag is invisible until
		// something tells it to rebuild that list. UStateTreeEditorMode listens for this and forces a
		// details refresh; broadcasting for an asset with no open editor is a no-op.
		UE::StateTree::Delegates::OnParametersChanged.Broadcast(StateTree);

		UE_LOG(LOG_GASC_StateTreeAbilityParams, Log,
			TEXT("Refreshed %d Grant Ability Dynamic node(s) in '%s' after a Blueprint compile."),
			NumChanged, *StateTree.GetPathName());
	}

	return NumChanged;
}

int32 UGASC_StateTreeAbilityParamsRefresher::RefreshLoadedStateTrees()
{
	int32 NumAssetsChanged = 0;

	// Only assets already in memory are considered. Anything loaded later runs the task's own
	// PostLoad sync on the way in, so scanning the asset registry and loading trees just to refresh
	// them would be work for no benefit.
	for (UStateTree* StateTree : TObjectRange<UStateTree>(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (!IsValid(StateTree) || StateTree->GetPackage() == GetTransientPackage())
		{
			continue;
		}

		NumAssetsChanged += RefreshStateTree(*StateTree) > 0 ? 1 : 0;
	}

	return NumAssetsChanged;
}
