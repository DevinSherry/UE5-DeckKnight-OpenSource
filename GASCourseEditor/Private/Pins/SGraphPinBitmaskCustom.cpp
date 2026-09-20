// Fill out your copyright notice in the Description page of Project Settings.

#include "Pins/SGraphPinBitmaskCustom.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/CString.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

void SGraphPinBitmaskCustom::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPinNum<int32>::Construct(SGraphPinNum<int32>::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SGraphPinBitmaskCustom::GetDefaultValueWidget()
{
	check(GraphPinObj);

	const FName PinSubCategory = GraphPinObj->PinType.PinSubCategory;
	const UEnum* BitmaskEnum = Cast<UEnum>(GraphPinObj->PinType.PinSubCategoryObject.Get());

	if (PinSubCategory == UEdGraphSchema_K2::PSC_Bitmask)
	{
		struct FBitmaskFlagInfo
		{
			int32 Value;
			FText DisplayName;
			FText ToolTipText;
		};

		const int32 BitmaskBitCount = sizeof(int32) << 3;
		TArray<FBitmaskFlagInfo> BitmaskFlags;
		BitmaskFlags.Reserve(BitmaskBitCount);

		if (BitmaskEnum)
		{
			const bool bUseEnumValuesAsMaskValues = BitmaskEnum->GetBoolMetaData(FBlueprintMetadata::MD_UseEnumValuesAsMaskValuesInEditor);
			auto AddNewBitmaskFlagLambda = [BitmaskEnum, &BitmaskFlags](int32 InEnumIndex, int32 InFlagValue)
			{
				FBitmaskFlagInfo& BitmaskFlag = BitmaskFlags.AddDefaulted_GetRef();
				BitmaskFlag.Value = InFlagValue;
				BitmaskFlag.DisplayName = BitmaskEnum->GetDisplayNameTextByIndex(InEnumIndex);
				BitmaskFlag.ToolTipText = BitmaskEnum->GetToolTipTextByIndex(InEnumIndex);
				if (BitmaskFlag.ToolTipText.IsEmpty())
				{
					BitmaskFlag.ToolTipText = FText::Format(NSLOCTEXT("GraphEditor", "BitmaskDefaultFlagToolTipText", "Toggle {0} on/off"), BitmaskFlag.DisplayName);
				}
			};

			for (int32 BitmaskEnumIndex = 0; BitmaskEnumIndex < BitmaskEnum->NumEnums() - 1; ++BitmaskEnumIndex)
			{
				const int64 EnumValue = BitmaskEnum->GetValueByIndex(BitmaskEnumIndex);
				const bool bIsHidden = BitmaskEnum->HasMetaData(TEXT("Hidden"), BitmaskEnumIndex);
				if (EnumValue >= 0 && !bIsHidden)
				{
					if (bUseEnumValuesAsMaskValues)
					{
						if (EnumValue < MAX_int32 && FMath::IsPowerOfTwo(EnumValue))
						{
							AddNewBitmaskFlagLambda(BitmaskEnumIndex, static_cast<int32>(EnumValue));
						}
					}
					else if (EnumValue < BitmaskBitCount)
					{
						AddNewBitmaskFlagLambda(BitmaskEnumIndex, 1 << static_cast<int32>(EnumValue));
					}
				}
			}
		}
		else
		{
			for (int32 BitmaskFlagIndex = 0; BitmaskFlagIndex < BitmaskBitCount; ++BitmaskFlagIndex)
			{
				FBitmaskFlagInfo& BitmaskFlag = BitmaskFlags.AddDefaulted_GetRef();
				BitmaskFlag.Value = static_cast<int32>(1 << BitmaskFlagIndex);
				BitmaskFlag.DisplayName = FText::Format(NSLOCTEXT("GraphEditor", "BitmaskDefaultFlagDisplayName", "Flag {0}"), FText::AsNumber(BitmaskFlagIndex + 1));
				BitmaskFlag.ToolTipText = FText::Format(NSLOCTEXT("GraphEditor", "BitmaskDefaultFlagToolTipText", "Toggle {0} on/off"), BitmaskFlag.DisplayName);
			}
		}

		const auto GetComboButtonText = [this, BitmaskFlags]() -> FText
		{
			const int32 BitmaskValue = FCString::Atoi(*GraphPinObj->GetDefaultAsString());
			if (BitmaskValue != 0)
			{
				TArray<FText> SelectedFlagDisplayNames;
				for (const FBitmaskFlagInfo& FlagInfo : BitmaskFlags)
				{
					if (FlagInfo.Value != 0 && (BitmaskValue & FlagInfo.Value) == FlagInfo.Value)
					{
						SelectedFlagDisplayNames.Add(FlagInfo.DisplayName);
					}
				}

				if (SelectedFlagDisplayNames.Num() > 0)
				{
					return FText::Join(FText::FromString(TEXT(" | ")), SelectedFlagDisplayNames);
				}
			}

			return NSLOCTEXT("GraphEditor", "BitmaskButtonContentNoFlagsSet", "(No Flags)");
		};

		return SNew(SComboButton)
			.ContentPadding(3.0f)
			.MenuPlacement(MenuPlacement_BelowAnchor)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			.ButtonContent()
			[
				SNew(SBox)
				.MinDesiredWidth(84.0f)
				[
					SNew(STextBlock)
					.Text_Lambda(GetComboButtonText)
					.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
				]
			]
			.OnGetMenuContent_Lambda([this, BitmaskFlags]()
			{
				FMenuBuilder MenuBuilder(false, nullptr);
				for (int32 i = 0; i < BitmaskFlags.Num(); ++i)
				{
					MenuBuilder.AddMenuEntry(
						BitmaskFlags[i].DisplayName,
						BitmaskFlags[i].ToolTipText,
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([this, BitmaskFlags, i]()
							{
								const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangePinValue", "Change Pin Value"));
								const int32 CurValue = FCString::Atoi(*GraphPinObj->GetDefaultAsString());
								GraphPinObj->Modify();
								GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FString::FromInt(CurValue ^ BitmaskFlags[i].Value));
							}),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda([this, BitmaskFlags, i]() -> bool
							{
								const int32 CurValue = FCString::Atoi(*GraphPinObj->GetDefaultAsString());
								return (CurValue & BitmaskFlags[i].Value) != 0;
							})
						),
						NAME_None,
						EUserInterfaceActionType::Check);
				}
				return MenuBuilder.MakeWidget();
			});
	}

	return SGraphPinNum<int32>::GetDefaultValueWidget();
}
