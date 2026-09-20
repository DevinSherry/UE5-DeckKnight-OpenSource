#pragma once

#include "AbilitySystemComponent.h"
#include "StateTreeTaskBase.h"
#include "Abilities/GameplayAbility.h"
#include "StructUtils/PropertyBag.h"
#include "STT_GrantAbilityDynamic.generated.h"

GASCOURSE_API DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_GrantAbilityTask, Log, All);

/**
 * StateTree Task: Grant a Gameplay Ability and expose its editable properties
 * dynamically as bindable pins.
 */

/** Instance Data holds per-node state and bindable properties */
USTRUCT()
struct FInstanceDataAbilityData
{
	GENERATED_BODY()

	/** Auto-generated property bag of ability parameters. */
	UPROPERTY(EditAnywhere, Category="Ability")
	FInstancedPropertyBag AbilityParams;

	/** Delegate binding handle for OnAbilityEnded */
	FDelegateHandle AbilityEndedDelegateHandle;

	/** Delegate binding handle for OnAbilityFailed */
	//FAbilityFailedDelegate AbilityFailedDelegateHandle;
	FDelegateHandle AbilityFailedDelegateHandle;

	bool bAbilityCancelled = false;
	bool bAbilityFailed = false;

	FGameplayAbilitySpecHandle GrantedHandle;
	bool bGrantedByTask   = false;
	bool bActivated       = false;
	bool bAbilityEnded    = false;

};

// Exported because the editor module's refresh subsystem reaches in to re-sync these nodes when an
// ability Blueprint is recompiled; without the macro StaticStruct() and the sync entry point below
// are not visible outside GASCourse.
USTRUCT()
struct GASCOURSE_API FGrantAbilityDynamicTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FGrantAbilityDynamicTask();

	/** The Gameplay Ability class to grant. */
	UPROPERTY(EditAnywhere, Category="Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	using FInstanceDataType = FInstanceDataAbilityData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataAbilityData::StaticStruct(); }

	// === Task lifecycle ===
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	// === Property bag schema ===
	//
	// The bag exposed on the instance data mirrors the ability class's own designer-facing
	// properties, so it goes stale the moment somebody adds a variable to that class. These three
	// functions are the self-healing set modelled on FStateTreeReference: describe what the bag
	// *should* look like, cheaply detect that it doesn't, and rebuild it while keeping whatever the
	// designer had already set. Everything that can trigger a rebuild routes through them.

	/**
	 * Reflects InAbilityClass into the property bag descriptors the task should be exposing.
	 *
	 * Walks the whole class chain up to but not including UGameplayAbility, so a designer sees the
	 * variables an ability inherits from the project's own base classes without also being offered
	 * the engine's ability plumbing.
	 *
	 * Each descriptor is given an ID derived deterministically from the property name. UPropertyBag
	 * memoises bag layouts by a hash that includes those IDs, and the default descriptor
	 * constructor leaves the ID blank for GetOrCreateFromDescs to fill in with a fresh random GUID.
	 * Left alone, a bag rebuilt this session could never hash to the same layout as the one loaded
	 * from disk, which would make RequiresAbilityParamsSync report "stale" forever.
	 */
	static void BuildAbilityParamDescs(const UClass* InAbilityClass,
		TArray<FPropertyBagPropertyDesc>& OutDescs);

	/** True when Bag's layout no longer matches what InAbilityClass currently declares. Cheap: one pointer compare. */
	static bool RequiresAbilityParamsSync(const FInstancedPropertyBag& Bag, const UClass* InAbilityClass);

	/**
	 * Rebuilds Bag to match InAbilityClass, preserving values for properties that survived the
	 * change and seeding newly appeared ones from the ability's own defaults.
	 * @return true if the layout actually changed.
	 */
	static bool SyncAbilityParams(FInstancedPropertyBag& Bag, const UClass* InAbilityClass);

	/** Syncs the instance data's bag if it is stale, logging that it did so. @return true if it changed. */
	bool ConditionallySyncAbilityParams(FStateTreeDataView InstanceDataView) const;

	/**
	 * Rebuilds a stale bag on load, which is what removes the need for any manual rebuild step:
	 * reopening a State Tree that binds to an ability class whose variables have since changed is
	 * enough to bring the pin list back in line. UStateTree::PostLoad runs this pass before Link(),
	 * so the binding copies are resolved against the layout we leave behind.
	 *
	 * The rebuild is deliberately editor-only even though this hook is not. See the definition.
	 */
	virtual void PostLoad(FStateTreeDataView InstanceDataView) override;

#if WITH_EDITOR
	/** Regenerate the property bag schema when AbilityClass changes. */
	virtual void PostEditNodeChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent, FStateTreeDataView InstanceDataView) override;

	/** Last line of defence: syncs the copy being baked so a compile can never bake a stale layout. */
	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;
#endif
};
