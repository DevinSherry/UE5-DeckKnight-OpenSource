// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Tasks/STT_GrantAbilityDynamic.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Game/GameplayAbilitySystem/GameplayAbilities/GASC_AbilityParamsObject.h"
#include "GASCourse/GASCourseCharacter.h"
#include "StructUtils/PropertyBag.h"
#include "Engine/BlueprintGeneratedClass.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

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
            UGASC_AbilityParamsObject* Params =
                NewObject<UGASC_AbilityParamsObject>(Pawn);

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

        // Ability already existed — reuse it
        Data.GrantedHandle = Spec->Handle;
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
    	UGameplayAbility* AbilityCDO = Spec->Ability;
        if (!Data.bActivated && AbilityCDO->CanActivateAbility(Spec->Handle,
        	ASC->AbilityActorInfo.Get(), nullptr, nullptr))
        {
            ASC->TryActivateAbility(Spec->Handle);
            Data.bActivated = true;
        }

        return EStateTreeRunStatus::Running;
    }

    // --------------------------------------------------
    // 6. Finish when ability ends
    // --------------------------------------------------


    return EStateTreeRunStatus::Running;
}



#if WITH_EDITOR

void FGrantAbilityDynamicTask::PostEditNodeChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent,
	FStateTreeDataView InstanceDataView)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGrantAbilityDynamicTask, AbilityClass))
	{
		if (FInstanceDataAbilityData* Data = InstanceDataView.GetMutablePtr<FInstanceDataAbilityData>())
		{
			UpdateAbilityParamsSchema(Data->AbilityParams, AbilityClass);
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGrantAbilityDynamicTask, bForceRebuild))
	{
		if (bForceRebuild)
		{
			if (FInstanceDataAbilityData* Data = InstanceDataView.GetMutablePtr<FInstanceDataAbilityData>())
			{
				UpdateAbilityParamsSchema(Data->AbilityParams, AbilityClass);
				bForceRebuild = false;
			}
		}
	}

	FStateTreeTaskCommonBase::PostEditNodeChangeChainProperty(PropertyChangedEvent, InstanceDataView);
}
#endif

void FGrantAbilityDynamicTask::UpdateAbilityParamsSchema(
    FInstancedPropertyBag& Bag,
    TSubclassOf<UGameplayAbility> InAbilityClass)
{
    if (!InAbilityClass)
    {
        Bag.Reset();
        return;
    }

    UGameplayAbility* AbilityCDO =
        InAbilityClass->GetDefaultObject<UGameplayAbility>();

    if (!AbilityCDO)
    {
        Bag.Reset();
        return;
    }

    TArray<FPropertyBagPropertyDesc> Descs;

    if (UBlueprintGeneratedClass* BPClass =
        Cast<UBlueprintGeneratedClass>(InAbilityClass))
    {
        for (TFieldIterator<FProperty> It(
                 BPClass, EFieldIteratorFlags::IncludeSuper);
             It; ++It)
        {
            FProperty* Prop = *It;

            // Only Blueprint-visible editable properties
            if (!Prop->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_Edit))
                continue;

            // Skip per-instance-disabled
            if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
                continue;

            // Skip deprecated
            if (Prop->HasAnyPropertyFlags(CPF_Deprecated))
                continue;

            FPropertyBagPropertyDesc Desc(Prop->GetFName(), Prop);
            if (Desc.ValueType != EPropertyBagPropertyType::None)
            {
                Descs.Add(MoveTemp(Desc));
            }
        }
    }

    const UPropertyBag* BagStruct =
        UPropertyBag::GetOrCreateFromDescs(Descs);

    Bag.InitializeFromBagStruct(BagStruct);

    const UPropertyBag* PBStruct = Bag.GetPropertyBagStruct();
    if (!PBStruct)
        return;

    for (const FPropertyBagPropertyDesc& Desc :
         PBStruct->GetPropertyDescs())
    {
        FProperty* SrcProp =
            AbilityCDO->GetClass()->FindPropertyByName(Desc.Name);

        if (!SrcProp)
            continue;

        void* SrcPtr =
            SrcProp->ContainerPtrToValuePtr<void>(AbilityCDO);

        if (!SrcPtr)
            continue;

        EPropertyBagResult Res =
            Bag.SetValue(Desc.Name, SrcProp, AbilityCDO);

        if (Res != EPropertyBagResult::Success)
        {
            // Optional: log mismatch
        }
    }
}
