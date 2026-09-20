// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorSubsystem.h"
#include "GASC_StateTreeAbilityParamsRefresher.generated.h"

class UStateTree;

GASCOURSEEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_StateTreeAbilityParams, Log, All);

/**
 * Keeps Grant Ability Dynamic task nodes in step with the ability classes they reflect.
 *
 * The task exposes an ability class's editable properties as bindable pins by reflecting that class
 * into a property bag. The bag is saved with the State Tree asset, so it is a snapshot: adding a
 * variable to the ability class leaves every task that grants it describing the class as it used to
 * be. The task heals itself whenever it is loaded, edited or compiled, which covers reopening an
 * asset, but none of those fire while a designer adds a variable to an ability Blueprint with the
 * State Tree already open in another tab - the case that previously needed a manual rebuild.
 *
 * This subsystem closes that gap by re-syncing the task nodes of every loaded State Tree after a
 * Blueprint compile, then asking the open editors to repaint. Together with the task's own three
 * hooks, there is no longer any path by which the pin list can be visibly out of date.
 */
UCLASS()
class UGASC_StateTreeAbilityParamsRefresher : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Re-syncs the Grant Ability Dynamic nodes of every State Tree currently in memory and refreshes
	 * the editors showing them.
	 *
	 * Only touches assets that are actually stale, and deliberately does not dirty them: the bag is
	 * rebuilt from the ability class either way on the next load, so forcing a save on every asset
	 * that happens to be in memory when an unrelated Blueprint compiles would be noise in source
	 * control. An edit the designer then makes to the refreshed pins dirties the asset as normal.
	 *
	 * @return the number of State Tree assets that needed changing.
	 */
	static int32 RefreshLoadedStateTrees();

private:
	void HandleBlueprintCompiled();

	/** @return the number of nodes re-synced within this asset. */
	static int32 RefreshStateTree(UStateTree& StateTree);

	FDelegateHandle BlueprintCompiledHandle;
};
