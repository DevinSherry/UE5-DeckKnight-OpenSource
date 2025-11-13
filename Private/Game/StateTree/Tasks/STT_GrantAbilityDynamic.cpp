// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/StateTree/Tasks/STT_GrantAbilityDynamic.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GameplayAbilities/GASC_AbilityParamsObject.h"
#include "GASCourse/GASCourseCharacter.h"
#include "StructUtils/PropertyBag.h"
#include "Engine/BlueprintGeneratedClass.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

EStateTreeRunStatus FGrantAbilityDynamicTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult&) const
{
	FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);

	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No AbilityClass set."));
		return EStateTreeRunStatus::Failed;
	}

	// Find ASC on the StateTree owner actor
	AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner actor."));
		return EStateTreeRunStatus::Failed;
	}
	AController* Controller = Cast<AController>(OwnerActor);
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner controller."));
		return EStateTreeRunStatus::Failed;
	}

	AGASCourseCharacter* Character = Cast<AGASCourseCharacter>(Controller->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner actor."));
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No AbilitySystemComponent on %s."), *OwnerActor->GetName());
		return EStateTreeRunStatus::Failed;
	}
	

	UGASC_AbilityParamsObject* ParamsObject = NewObject<UGASC_AbilityParamsObject>(OwnerActor);
	ParamsObject->InitFromBag(Data.AbilityParams);

	// Grant the ability
	FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, ParamsObject);
	FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
	FGameplayAbilitySpec* GivenSpec = ASC->FindAbilitySpecFromHandle(Handle);

	if (!Handle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: Failed to give ability %s"), *AbilityClass->GetName());
		return EStateTreeRunStatus::Failed;
	}

	Data.GrantedAbilityHandle = Handle;
	
	Data.AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddLambda(
			[SpecHandle = Handle, &Data](const FAbilityEndedData& EndedData)
			{
				if (EndedData.AbilitySpecHandle == SpecHandle)
				{
					UE_LOG(LogTemp, Log, TEXT("Ability %s ended"), *EndedData.AbilityThatEnded->GetName());
					Data.bAbilityEnded = true;
				}
			});
	
	Data.AbilityFailedDelegateHandle = ASC->AbilityFailedCallbacks.AddLambda(
		[SpecHandle = Handle, &Data](const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailedTags)
		{
			if (FailedAbility->GetCurrentAbilitySpec()->Handle == SpecHandle)
			{
				UE_LOG(LogTemp, Log, TEXT("Ability %s failed"), *FailedAbility->GetName());
				Data.bAbilityEnded = true;
			}
		});
	

	// Optionally activate immediately
	ASC->TryActivateAbility(Handle);

	return EStateTreeRunStatus::Running;
}

void FGrantAbilityDynamicTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);

	AController* Controller = Cast<AController>(Context.GetOwner());
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner controller."));
		return;
	}

	AGASCourseCharacter* Character = Cast<AGASCourseCharacter>(Controller->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner actor."));
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No AbilitySystemComponent on %s."), *Character->GetName());
		return;
	}

	if (Data.GrantedAbilityHandle.IsValid())
	{
		ASC->ClearAbility(Data.GrantedAbilityHandle);
		Data.GrantedAbilityHandle = FGameplayAbilitySpecHandle(); // reset
	}
}

EStateTreeRunStatus FGrantAbilityDynamicTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataAbilityData& Data = Context.GetInstanceData(*this);
	
	AController* Controller = Cast<AController>(Context.GetOwner());
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner controller."));
		return EStateTreeRunStatus::Failed;
	}

	AGASCourseCharacter* Character = Cast<AGASCourseCharacter>(Controller->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No owner actor."));
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantAbilityDynamic: No AbilitySystemComponent on %s."), *Character->GetName());
		return EStateTreeRunStatus::Failed;
	}

	if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Data.GrantedAbilityHandle))
	{
		if (Spec->IsActive())
		{
			return EStateTreeRunStatus::Running;
		}
		
		UE_LOG(LogTemp, Log, TEXT("Ability ended (polled)"));
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
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

void FGrantAbilityDynamicTask::UpdateAbilityParamsSchema(FInstancedPropertyBag& Bag, TSubclassOf<UGameplayAbility> InAbilityClass)
{
	if (!InAbilityClass)
	{
		Bag.Reset();
		return;
	}

	UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();
	if (!AbilityCDO)
	{
		Bag.Reset();
		return;
	}

	TArray<FPropertyBagPropertyDesc> Descs;

	//const UClass* Class = InAbilityClass.Get();

	if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(AbilityClass))
	{
		for (TFieldIterator<FProperty> It(BPClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			FProperty* Prop = *It;
			UClass* PropOwner = Prop->GetOwner<UClass>();

			// Skip if property owner is not a Blueprint class (i.e., it's a C++ class)
			if (!PropOwner || !Cast<UBlueprintGeneratedClass>(PropOwner))
			{
				continue; // Skip C++ properties
			}

			// Only include properties that are editable/visible in Blueprint
			if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
				continue;
			
			// Skip properties that are marked as DisableEditOnInstance
			if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
				continue;

			// Skip deprecated properties
			if (Prop->HasAnyPropertyFlags(CPF_Deprecated))
				continue;

			UE_LOG(LogTemp, Log, TEXT("Adding BP property: %s from class: %s"), 
				*Prop->GetName(), 
				*PropOwner->GetName());

		
			FPropertyBagPropertyDesc Desc(Prop->GetFName(), Prop);
			if (Desc.ValueType != EPropertyBagPropertyType::None)
			{
				Descs.Add(MoveTemp(Desc));
			}
		}
	}
	
	const UPropertyBag* BagStruct = UPropertyBag::GetOrCreateFromDescs(Descs);
	Bag.InitializeFromBagStruct(BagStruct);

	for (const FPropertyBagPropertyDesc& Desc : Bag.GetPropertyBagStruct()->GetPropertyDescs())
	{
		if (FProperty* SrcProp = AbilityCDO->GetClass()->FindPropertyByName(Desc.Name))
		{
			void* SrcPtr = SrcProp->ContainerPtrToValuePtr<void>(AbilityCDO);
			if (!SrcPtr) continue;

			const UPropertyBag* PBStruct = Bag.GetPropertyBagStruct();
			if (!PBStruct)
			{
				return;
			}

			const FPropertyBagPropertyDesc* BagDesc = PBStruct->FindPropertyDescByName(Desc.Name);
			if (!BagDesc)
			{
				return;
			}

			// Get the destination address inside the bag and copy into it

			// Copy from SrcProp/SrcPtr into the bag using the public API
			EPropertyBagResult Res = Bag.SetValue(BagDesc->Name, SrcProp, AbilityCDO /* e.g., Ability */);
			if (Res != EPropertyBagResult::Success)
			{
				// handle mismatch or missing property if needed
			}
		}
	}
}

