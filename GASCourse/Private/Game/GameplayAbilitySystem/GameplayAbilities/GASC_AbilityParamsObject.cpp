// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameplayAbilitySystem/GameplayAbilities/GASC_AbilityParamsObject.h"

DEFINE_LOG_CATEGORY(LOG_GASC_AbilityParams);

namespace
{
	/**
	 * True when SrcProp's bytes can be safely read as DestProp's type.
	 *
	 * SameType covers the overwhelming majority: it compares the property class and the
	 * type-specific payload, so FStructProperty agreement means the same UScriptStruct,
	 * FObjectProperty agreement means the same class, and so on.
	 *
	 * The fallback exists for enums. A property bag always materialises an enum as an
	 * FEnumProperty, but an ability that declares TEnumAsByte<EFoo> has an FByteProperty with an
	 * Enum set. Those describe the same value and the same bytes, yet SameType rejects the pair
	 * because the property classes differ. Comparing them as bag descriptors instead - which
	 * normalise both spellings to EPropertyBagPropertyType::Enum pointing at the same UEnum - lets
	 * that case through, with a size check so the byte copy can never run past either side.
	 */
	bool IsCopyCompatible(const FProperty* SrcProp, const FProperty* DestProp)
	{
		if (!SrcProp || !DestProp)
		{
			return false;
		}

		if (DestProp->SameType(SrcProp))
		{
			return true;
		}

		const FPropertyBagPropertyDesc SrcDesc(SrcProp->GetFName(), SrcProp);
		const FPropertyBagPropertyDesc DestDesc(DestProp->GetFName(), DestProp);

		return SrcDesc.ValueType != EPropertyBagPropertyType::None
			&& SrcDesc.CompatibleType(DestDesc)
			&& SrcProp->GetSize() == DestProp->GetSize();
	}
}

int32 UGASC_AbilityParamsObject::ApplyTo(UObject* Target) const
{
	if (!Target)
	{
		return 0;
	}

	const UPropertyBag* BagStruct = Params.GetPropertyBagStruct();
	if (!BagStruct)
	{
		// Nothing was bound on the granting side. Normal, not a problem.
		return 0;
	}

	const FConstStructView BagValue = Params.GetValue();
	const uint8* BagMemory = BagValue.GetMemory();
	if (!BagMemory)
	{
		return 0;
	}

	const UClass* TargetClass = Target->GetClass();
	int32 NumApplied = 0;

	for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
	{
		const FProperty* SrcProp = Desc.CachedProperty;
		if (!SrcProp)
		{
			continue;
		}

		const FProperty* DestProp = TargetClass->FindPropertyByName(Desc.Name);
		if (!DestProp)
		{
			// The property was removed or renamed on the ability class since the bag was authored.
			UE_LOG(LOG_GASC_AbilityParams, Verbose,
				TEXT("'%s' has no property named '%s'; skipping that ability parameter."),
				*TargetClass->GetName(), *Desc.Name.ToString());
			continue;
		}

		if (!IsCopyCompatible(SrcProp, DestProp))
		{
			UE_LOG(LOG_GASC_AbilityParams, Warning,
				TEXT("Ability parameter '%s' on '%s' is declared as %s but the granting bag holds %s; ")
				TEXT("skipping it rather than copying mismatched bytes. Re-open the asset that grants this ability ")
				TEXT("so its parameter list can rebuild against the current class."),
				*Desc.Name.ToString(), *TargetClass->GetName(),
				*DestProp->GetCPPType(), *SrcProp->GetCPPType());
			continue;
		}

		const void* SrcPtr = SrcProp->ContainerPtrToValuePtr<void>(BagMemory);
		void* DstPtr = DestProp->ContainerPtrToValuePtr<void>(Target);

		if (!SrcPtr || !DstPtr)
		{
			continue;
		}

		DestProp->CopyCompleteValue(DstPtr, SrcPtr);
		++NumApplied;
	}

	UE_LOG(LOG_GASC_AbilityParams, Verbose,
		TEXT("Applied %d of %d ability parameter(s) to '%s'."),
		NumApplied, BagStruct->GetPropertyDescs().Num(), *Target->GetName());

	return NumApplied;
}
