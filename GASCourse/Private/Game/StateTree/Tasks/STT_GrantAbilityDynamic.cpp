// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Tasks/STT_GrantAbilityDynamic.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Game/GameplayAbilitySystem/GameplayAbilities/GASC_AbilityParamsObject.h"
#include "GASCourse/GASCourseCharacter.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "StructUtils/PropertyBag.h"
#include "Algo/Reverse.h"
#include "Misc/Guid.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

#define LOCTEXT_NAMESPACE "GrantAbilityDynamicTask"

DEFINE_LOG_CATEGORY(LOG_GASC_GrantAbilityTask);

namespace GASCourse::GrantAbilityDynamic
{
	/**
	 * Properties declared by UGameplayAbility and above are the engine's own ability plumbing -
	 * instancing policy, cost and cooldown effect classes, replication settings and the like. They
	 * are editable, so a naive reflection pass picks them all up, but none of them is a per-grant
	 * designer knob and letting a State Tree overwrite them at grant time would quietly break GAS.
	 * Reflection therefore stops as soon as it reaches this class.
	 */
	static const UClass* GetReflectionStopClass()
	{
		return UGameplayAbility::StaticClass();
	}

	/** True when Struct came from compiling a Blueprint rather than from a C++ declaration. */
	static bool IsBlueprintGenerated(const UStruct* Struct)
	{
		return Struct && Struct->IsA<UBlueprintGeneratedClass>();
	}

	/**
	 * True for properties worth offering as a bindable pin on the task.
	 *
	 * The two halves of the rule are deliberately different, because "the author said this is part
	 * of the ability's interface" is spelled differently in each language.
	 *
	 * In C++ it is BlueprintReadWrite and nothing else. That is the one declaration which states
	 * both that the value is public to content and that content is allowed to write it, which is
	 * exactly the contract this task needs - it writes the value into a live ability at grant time.
	 * Notably an EditDefaultsOnly or BlueprintReadOnly knob is *not* offered: those are class-level
	 * tuning the author chose not to expose for writing, and quietly overwriting them per grant
	 * would break that intent.
	 *
	 * In Blueprint there is no read-only variable, so the equivalent is the pair of settings the
	 * variable editor offers: it must be ticked "Instance Editable" *and* be public, meaning the
	 * "Private" checkbox under Advanced is clear. Both are required. Either one alone lets through
	 * almost the entire class - a Blueprint variable is public unless somebody went and ticked
	 * Private, so "public" on its own is barely a filter at all and internal scratch variables
	 * sail past it.
	 *
	 * @param Prop                 the candidate property.
	 * @param bDeclaredInBlueprint whether the declaring class is a UBlueprintGeneratedClass.
	 */
	static bool IsExposableAbilityParam(const FProperty* Prop, const bool bDeclaredInBlueprint)
	{
		if (!Prop)
		{
			return false;
		}

		// Bookkeeping, editor-only and dead fields are never meaningful to copy into a live ability.
		if (Prop->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient | CPF_DuplicateTransient | CPF_EditorOnly))
		{
			return false;
		}

		if (!bDeclaredInBlueprint)
		{
			// UPROPERTY(BlueprintReadWrite) is CPF_BlueprintVisible without CPF_BlueprintReadOnly.
			return Prop->HasAnyPropertyFlags(CPF_BlueprintVisible)
				&& !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly);
		}

		// "Instance Editable" is stored as the absence of CPF_DisableEditOnInstance - see
		// FBlueprintVarActionDetails::OnEditableCheckboxState, which reads exactly this.
		const bool bInstanceEditable = Prop->HasAnyPropertyFlags(CPF_Edit)
			&& !Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if (!bInstanceEditable)
		{
			return false;
		}

#if WITH_METADATA
		// "Private", unlike every other Blueprint variable setting, is metadata rather than a
		// property flag: FBlueprintMetadata::MD_Private, spelled out here so this runtime module
		// does not have to depend on BlueprintGraph. Mirrors FBlueprintEditorUtils::IsPropertyPrivate.
		static const FName BlueprintPrivateMetaDataKey(TEXT("BlueprintPrivate"));

		const bool bPrivate = Prop->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPrivate)
			|| Prop->GetBoolMetaData(BlueprintPrivateMetaDataKey);

		return !bPrivate;
#else
		// Property metadata does not exist in a build without it, so the "Private" checkbox cannot be
		// read and the answer rests on the instance-editable flag alone, which has already said yes.
		// The resulting disagreement with the editor's filter is inert: every caller that could act on
		// it - PostEditNodeChangeChainProperty, Compile, PostLoad and the editor refresh subsystem -
		// is editor-only, so nothing outside the editor ever rebuilds a bag and no cooked layout can
		// drift away from the one that was authored.
		return true;
#endif
	}
}

FGrantAbilityDynamicTask::FGrantAbilityDynamicTask()
{
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bShouldStateChangeOnReselect = true;
#endif
}

EStateTreeRunStatus FGrantAbilityDynamicTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult&) const
{
	FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);

	Data.GrantedHandle    = FGameplayAbilitySpecHandle();
	Data.bGrantedByTask   = false;
	Data.bActivated       = false;
	Data.bAbilityEnded    = false;
	Data.AbilityEndedDelegateHandle.Reset();

	return EStateTreeRunStatus::Running;
}

void FGrantAbilityDynamicTask::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult&) const
{
	FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);

	AController* Controller = Cast<AController>(Context.GetOwner());
	if (!Controller)
		return;

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
		return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASI)
		return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
		return;

	// Remove delegate
	if (Data.AbilityEndedDelegateHandle.IsValid())
	{
		ASC->OnAbilityEnded.Remove(Data.AbilityEndedDelegateHandle);
		Data.AbilityEndedDelegateHandle.Reset();
	}

	// Remove ability ONLY if this task granted it
	if (Data.bGrantedByTask && Data.GrantedHandle.IsValid())
	{
		ASC->ClearAbility(Data.GrantedHandle);
		Data.GrantedHandle = FGameplayAbilitySpecHandle();
	}
}



EStateTreeRunStatus FGrantAbilityDynamicTask::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);

    if (!AbilityClass)
        return EStateTreeRunStatus::Failed;

    AController* Controller = Cast<AController>(Context.GetOwner());
    if (!Controller)
        return EStateTreeRunStatus::Failed;

    APawn* Pawn = Controller->GetPawn();
    if (!Pawn)
        return EStateTreeRunStatus::Running;

    IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
    if (!ASI)
        return EStateTreeRunStatus::Failed;

    UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
    if (!ASC)
        return EStateTreeRunStatus::Failed;

	if (Data.bAbilityEnded)
	{
		return EStateTreeRunStatus::Succeeded;
	}

    // --------------------------------------------------
    // 1. Wait for GAS readiness
    // --------------------------------------------------
    if (!ASC->AbilityActorInfo.IsValid())
        return EStateTreeRunStatus::Running;

    // --------------------------------------------------
    // 2. Resolve spec (NEVER trust cached handle alone)
    // --------------------------------------------------
    FGameplayAbilitySpec* Spec = nullptr;

    if (Data.GrantedHandle.IsValid())
    {
        Spec = ASC->FindAbilitySpecFromHandle(Data.GrantedHandle);
    }

    // --------------------------------------------------
    // 3. Grant if needed (once)
    // --------------------------------------------------
    if (!Spec)
    {
        // Check if ability already exists (spam-safe)
        Spec = ASC->FindAbilitySpecFromClass(AbilityClass);

        if (!Spec)
        {
            // The bag rides along as the spec's SourceObject; the ability reads it back in
            // OnGiveAbility and again in PreActivate. Outered to the ASC rather than the pawn so
            // it is guaranteed to outlive the spec that points at it.
            UGASC_AbilityParamsObject* Params =
                NewObject<UGASC_AbilityParamsObject>(ASC);

            Params->InitFromBag(Data.AbilityParams);

            FGameplayAbilitySpec NewSpec(
                AbilityClass,
                1,
                INDEX_NONE,
                Params);

            Data.GrantedHandle = ASC->GiveAbility(NewSpec);
            Data.bGrantedByTask = true;

            return EStateTreeRunStatus::Running;
        }

        // Ability already existed - reuse it. Its parameters came from whoever granted it, so this
        // task deliberately leaves them alone rather than reaching into someone else's spec.
        Data.GrantedHandle = Spec->Handle;

        UE_LOG(LOG_GASC_GrantAbilityTask, Verbose,
            TEXT("'%s' was already granted to '%s'; reusing the existing spec and leaving its parameters as granted."),
            *GetNameSafe(AbilityClass), *GetNameSafe(Pawn));
    }

    // --------------------------------------------------
    // 4. Bind end delegate once
    // --------------------------------------------------
    if (!Data.AbilityEndedDelegateHandle.IsValid())
    {
        Data.AbilityEndedDelegateHandle =
            ASC->OnAbilityEnded.AddLambda(
                [&Data](const FAbilityEndedData& Ended)
                {
                    if (Ended.AbilitySpecHandle == Data.GrantedHandle)
                    {
                        Data.bAbilityEnded = true;
                    }
                });
    }

    // --------------------------------------------------
    // 5. Activate once (spam-proof)
    // --------------------------------------------------
    if (!Spec->IsActive())
    {
    	// Spec->Ability is the class default object, not a live instance - it is only ever used
    	// here to ask the class whether activation is currently allowed.
    	const UGameplayAbility* AbilityDefault = Spec->Ability;
    	if (!AbilityDefault)
    	{
    		UE_LOG(LOG_GASC_GrantAbilityTask, Error,
    			TEXT("Ability spec for '%s' on '%s' has no ability object; cannot activate."),
    			*GetNameSafe(AbilityClass), *GetNameSafe(Pawn));
    		return EStateTreeRunStatus::Failed;
    	}

        if (!Data.bActivated && AbilityDefault->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get(), nullptr, nullptr))
        {
            ASC->TryActivateAbility(Spec->Handle);
            Data.bActivated = true;
        	return EStateTreeRunStatus::Running;
        }

    	UE_LOG(LOG_GASC_GrantAbilityTask, Verbose,
    		TEXT("'%s' on '%s' is not active and cannot activate (already attempted: %s); failing the task."),
    		*GetNameSafe(AbilityClass), *GetNameSafe(Pawn), Data.bActivated ? TEXT("yes") : TEXT("no"));
		return EStateTreeRunStatus::Failed;
    }

    // --------------------------------------------------
    // 6. Finish when ability ends
    // --------------------------------------------------

    return EStateTreeRunStatus::Running;
}

void FGrantAbilityDynamicTask::BuildAbilityParamDescs(
	const UClass* InAbilityClass,
	TArray<FPropertyBagPropertyDesc>& OutDescs)
{
	using namespace GASCourse::GrantAbilityDynamic;

	OutDescs.Reset();

	if (!InAbilityClass)
	{
		return;
	}

	// Gather the classes to reflect walking up from the chosen one, then reverse so properties come
	// out base-first. Field iteration order within a class is stable, so a stable class order makes
	// the whole descriptor array - and therefore the bag layout hash - stable too.
	TArray<const UStruct*, TInlineAllocator<8>> ClassChain;
	for (const UStruct* Current = InAbilityClass;
		Current && Current != GetReflectionStopClass();
		Current = Current->GetSuperStruct())
	{
		ClassChain.Add(Current);
	}
	Algo::Reverse(ClassChain);

	for (const UStruct* Current : ClassChain)
	{
		// Asked once per class rather than once per property: which language a property was
		// declared in is a fact about its declaring class, and that decides which half of the
		// exposure rule applies.
		const bool bDeclaredInBlueprint = IsBlueprintGenerated(Current);

		for (TFieldIterator<FProperty> It(Current, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			const FProperty* Prop = *It;

			if (!IsExposableAbilityParam(Prop, bDeclaredInBlueprint))
			{
				continue;
			}

			FPropertyBagPropertyDesc Desc(Prop->GetFName(), Prop);

			if (Desc.ValueType == EPropertyBagPropertyType::None)
			{
				// Property bags cannot represent every property type; skip what they cannot hold.
				continue;
			}

			// Give the descriptor a stable ID. UPropertyBag memoises layouts by a hash that
			// includes every descriptor's ID, and the (Name, FProperty) constructor leaves the ID
			// blank for GetOrCreateFromDescs to fill in with a fresh random GUID *after* hashing.
			// A bag deserialised from disk carries the random IDs it was saved with, so without
			// this a rebuilt descriptor array could never hash to the loaded layout and the staleness
			// check below would fire on every single load. Deriving the ID from the property name
			// also makes FPropertyBagPropertyDesc::operator== meaningful, which it is not when
			// every ID is the zero GUID.
			Desc.ID = FGuid::NewDeterministicGuid(Prop->GetFName().ToString());

#if WITH_EDITOR
			// Carry the authoring hints across so a generated pin gets the same tooltip and the
			// same slider/clamp range the property has on the ability class.
			static const FName MetaDataKeysToForward[] =
			{
				TEXT("ToolTip"),
				TEXT("ClampMin"),
				TEXT("ClampMax"),
				TEXT("UIMin"),
				TEXT("UIMax"),
				TEXT("Units"),
				TEXT("Categories")
			};

			for (const FName& Key : MetaDataKeysToForward)
			{
				if (Prop->HasMetaData(Key))
				{
					Desc.SetMetaData(Key, Prop->GetMetaData(Key));
				}
			}
#endif

			OutDescs.Add(MoveTemp(Desc));
		}
	}
}

bool FGrantAbilityDynamicTask::RequiresAbilityParamsSync(
	const FInstancedPropertyBag& Bag,
	const UClass* InAbilityClass)
{
	TArray<FPropertyBagPropertyDesc> Descs;
	BuildAbilityParamDescs(InAbilityClass, Descs);

	// Bag layouts are memoised by content, so the layout the class implies right now and the layout
	// the bag is holding are the same object whenever they agree. One pointer compare, no allocation
	// beyond the descriptor array, which is what makes this safe to call from PostLoad.
	const UPropertyBag* ExpectedBagStruct = Descs.IsEmpty()
		? nullptr
		: UPropertyBag::GetOrCreateFromDescs(Descs);

	return Bag.GetPropertyBagStruct() != ExpectedBagStruct;
}

bool FGrantAbilityDynamicTask::SyncAbilityParams(
	FInstancedPropertyBag& Bag,
	const UClass* InAbilityClass)
{
	TArray<FPropertyBagPropertyDesc> Descs;
	BuildAbilityParamDescs(InAbilityClass, Descs);

	const UPropertyBag* NewBagStruct = Descs.IsEmpty()
		? nullptr
		: UPropertyBag::GetOrCreateFromDescs(Descs);

	if (Bag.GetPropertyBagStruct() == NewBagStruct)
	{
		return false;
	}

	if (!NewBagStruct)
	{
		// No class selected, or nothing about it is exposable.
		Bag.Reset();
		return true;
	}

	const FInstancedPropertyBag OldBag = Bag;
	Bag.InitializeFromBagStruct(NewBagStruct);

	// Work out which values can survive the change: same name and a compatible type. Type-changed
	// properties are deliberately excluded here so they fall through to the CDO seeding below
	// rather than keeping a value that no longer means anything.
	TArray<FName> PreservedNames;
	PreservedNames.Reserve(NewBagStruct->GetPropertyDescs().Num());

	for (const FPropertyBagPropertyDesc& Desc : NewBagStruct->GetPropertyDescs())
	{
		const FPropertyBagPropertyDesc* OldDesc = OldBag.FindPropertyDescByName(Desc.Name);
		if (OldDesc && OldDesc->CompatibleType(Desc))
		{
			PreservedNames.Add(Desc.Name);
		}
	}

	Bag.CopyMatchingValuesByName(OldBag, MakeConstArrayView(PreservedNames));

	// Seed everything else - properties that just appeared, and any whose type changed - from the
	// ability's own defaults, so a fresh pin starts at the value the class author chose instead of
	// at zero.
	if (UObject* AbilityCDO = InAbilityClass ? InAbilityClass->GetDefaultObject() : nullptr)
	{
		for (const FPropertyBagPropertyDesc& Desc : NewBagStruct->GetPropertyDescs())
		{
			if (PreservedNames.Contains(Desc.Name))
			{
				continue;
			}

			if (const FProperty* SrcProp = AbilityCDO->GetClass()->FindPropertyByName(Desc.Name))
			{
				const EPropertyBagResult Result = Bag.SetValue(Desc.Name, SrcProp, AbilityCDO);
				if (Result != EPropertyBagResult::Success)
				{
					UE_LOG(LOG_GASC_GrantAbilityTask, Verbose,
						TEXT("Could not seed ability parameter '%s' from the '%s' defaults; it will start at its zero value."),
						*Desc.Name.ToString(), *GetNameSafe(InAbilityClass));
				}
			}
		}
	}

	return true;
}

bool FGrantAbilityDynamicTask::ConditionallySyncAbilityParams(FStateTreeDataView InstanceDataView) const
{
	FInstanceDataAbilityData* Data = InstanceDataView.GetMutablePtr<FInstanceDataAbilityData>();
	if (!Data)
	{
		return false;
	}

	if (!SyncAbilityParams(Data->AbilityParams, AbilityClass))
	{
		return false;
	}

	UE_LOG(LOG_GASC_GrantAbilityTask, Log,
		TEXT("Rebuilt the ability parameter list for '%s' to match the class's current properties."),
		*GetNameSafe(AbilityClass));

	return true;
}

void FGrantAbilityDynamicTask::PostLoad(FStateTreeDataView InstanceDataView)
{
	FStateTreeTaskCommonBase::PostLoad(InstanceDataView);

	// Unlike the other two rebuild hooks this one is not itself editor-only - UStateTree::PostLoad
	// runs the per-node pass in every configuration - so the rebuild is guarded here instead.
	//
	// Outside the editor there is nobody to heal the layout for: the ability classes are frozen at
	// cook time and the bag stored in the asset is by definition the one the bindings were compiled
	// against. Rebuilding it on load could only discard authored values or leave a binding pointing
	// at a property that no longer exists in the layout Link() is about to resolve against. It also
	// means the filter is free to consult editor-only property metadata, which a cooked build has
	// no way to read - see IsExposableAbilityParam.
#if WITH_EDITOR
	ConditionallySyncAbilityParams(InstanceDataView);
#endif
}

#if WITH_EDITOR

void FGrantAbilityDynamicTask::PostEditNodeChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent,
	FStateTreeDataView InstanceDataView)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGrantAbilityDynamicTask, AbilityClass))
	{
		ConditionallySyncAbilityParams(InstanceDataView);
	}

	FStateTreeTaskCommonBase::PostEditNodeChangeChainProperty(PropertyChangedEvent, InstanceDataView);
}

EDataValidationResult FGrantAbilityDynamicTask::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	if (!AbilityClass)
	{
		CompileContext.AddValidationError(LOCTEXT("MissingAbilityClass",
			"Grant Ability Dynamic has no Ability Class set, so it has nothing to grant."));
		return EDataValidationResult::Invalid;
	}

	// Compile runs against the duplicated data that will actually be baked, so this is the last
	// point at which a stale parameter layout can be caught before it ships.
	ConditionallySyncAbilityParams(CompileContext.GetInstanceDataView());

	return EDataValidationResult::Valid;
}

#endif

#undef LOCTEXT_NAMESPACE
