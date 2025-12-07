// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/GASCDamageEventsPanel.h"
#include "imgui.h"
#include "Game/Systems/Damage/Debug/DamagePipelineDebugSubsystem.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

//TODO
// Show 'No Valid Hit Result' text when there is no blocking hit? Or valid location (non-zero vector)
//Add checkbox to draw hit result debug shape in world.
//Simulate Damage doesn't take into account attributes or other context

TWeakObjectPtr<APawn> FGASCDamageEventsPanel::SelectedPawn = nullptr;
EGASC_DamagePipelineType FGASCDamageEventsPanel::SelectedPipelineType = Healing;

FGASCDamageEventsPanel::FGASCDamageEventsPanel()
{
}

FGASCDamageEventsPanel::~FGASCDamageEventsPanel()
{
}

void FGASCDamageEventsPanel::DrawDebugPanel(bool& bOpen)
{
	if (!bOpen)
	{
		return;
	}
	
	if (!ImGui::Begin("Damage Pipeline Damage Events", &bOpen))
	{
		ImGui::End();
		return;
	}
	FString OptionsTableId = FString::Printf(TEXT("Options##Table"));
	auto OptionsTableIdANSI = StringCast<ANSICHAR>(*OptionsTableId);

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
	if (ImGui::BeginTable(OptionsTableIdANSI.Get(), 2, ImGuiTableFlags_SizingFixedFit |
												 ImGuiTableFlags_Resizable |
												 ImGuiTableFlags_BordersV |
												 ImGuiTableFlags_Reorderable |
												 ImGuiTableFlags_Hideable |
												 ImGuiTableFlags_SizingStretchSame |
												 ImGuiTableColumnFlags_WidthStretch |
												 ImGuiTableFlags_BordersH |
												 ImGuiTableFlags_BordersH ))
	{
		ImGui::TableSetupColumn("Pawn Selection");
		ImGui::TableSetupColumn("Damage Modification Type");
		ImGui::TableHeadersRow();
		
		ImGui::TableNextColumn();
		
		// Pawn selection combo
		if (ImGui::BeginCombo("Pawn Selection", SelectedPawn.IsValid() ? TCHAR_TO_ANSI(*SelectedPawn->GetName()) : "None"))
		{
			for (TWeakObjectPtr<APawn> It : CachedPawns)
			{
				APawn* Pawn = It.Get();
				if (!IsValid(Pawn)) continue;

				bool bIsSelected = (Pawn == SelectedPawn.Get());
				FString PawnName = Pawn->GetName();

				if (ImGui::Selectable(TCHAR_TO_ANSI(*PawnName), bIsSelected))
				{
					SelectedPawn = Pawn;
					if (bIsSelected) ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		
		ImGui::TableNextColumn();
		
		// Pawn selection combo
		FString EnumAsString = StaticEnum<EGASC_DamagePipelineType>()->GetNameStringByValue((int64)SelectedPipelineType);
		const char* AnsiName = TCHAR_TO_ANSI(*EnumAsString);
		if (ImGui::BeginCombo("Modification Type", AnsiName))
		{
			// Loop through enum values
			UEnum* EnumPtr = StaticEnum<EGASC_DamagePipelineType>();
			int32 EnumCount = EnumPtr->NumEnums() - 1; // last entry is _MAX or hidden

			for (int32 i = 0; i < EnumCount; i++)
			{
				EGASC_DamagePipelineType EnumValue = (EGASC_DamagePipelineType)EnumPtr->GetValueByIndex(i);

				FString Name = EnumPtr->GetNameStringByIndex(i);
				const char* NameAnsi = TCHAR_TO_ANSI(*Name);

				bool bIsSelected = (EnumValue == SelectedPipelineType);

				if (ImGui::Selectable(NameAnsi, bIsSelected))
				{
					SelectedPipelineType = EnumValue;

					if (bIsSelected)
						ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	
	if (SelectedPawn.IsValid())
	{
		if (UDamagePipelineDebugSubsystem* DamagePipelineDebugSubsystem = SelectedPawn->GetWorld()->GetSubsystem<UDamagePipelineDebugSubsystem>())
		{
			FString TableId = FString::Printf(TEXT("%s##Table"), *SelectedPawn->GetName());
			auto TableIdANSI = StringCast<ANSICHAR>(*TableId);

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
			if (ImGui::BeginTable(TableIdANSI.Get(), 6, ImGuiTableFlags_SizingFixedFit |
														 ImGuiTableFlags_Resizable |
														 ImGuiTableFlags_BordersV |
														 ImGuiTableFlags_Reorderable |
														 ImGuiTableFlags_Hideable |
														 ImGuiTableFlags_SizingStretchSame |
														 ImGuiTableColumnFlags_WidthStretch |
														 ImGuiTableFlags_BordersH |
														 ImGuiTableFlags_BordersH ))
			{
				ImGui::TableSetupColumn("Instigator Info");
				ImGui::TableSetupColumn("Target Info");
				ImGui::TableSetupColumn("Context Data");
				ImGui::TableSetupColumn("Modification Values");
				ImGui::TableSetupColumn("Hit Result");
				ImGui::TableSetupColumn("Simulate Damage");
				ImGui::TableHeadersRow();
			}
			
			ImVec4 green = ImVec4(0, 1, 0, 1);
			ImVec4 red   = ImVec4(1, 0, 0, 1);
			ImVec4 white = ImVec4(1, 1, 1, 1);
			ImVec4 attributecolor = ImVec4(1, 1, 0, 1);
			ImVec4 gameplaytagcolor = ImVec4(0, 1, 1, 1);
				
			TArray<FDamageLogEntry> DamageLogEntries = DamagePipelineDebugSubsystem->GetDamageLogEntriesForActorID(SelectedPawn->GetUniqueID());
			for (FDamageLogEntry& Entry : DamageLogEntries)
			{
				if (Entry.bIsDamageEffect && SelectedPipelineType != Damage || !Entry.bIsDamageEffect && SelectedPipelineType != Healing)
				{
					continue;
				}
				FString DamageModificationCategory = Entry.bIsDamageEffect ? "Damage" : "Healing";
				ImGui::TableNextColumn();
				
				float colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
				ImGui::PushTextWrapPos(colEnd);
				ImGui::Text("%s: Instigator Actor: %s", TCHAR_TO_ANSI(*DamageModificationCategory), TCHAR_TO_ANSI(*Entry.HitInstigatorName));
				ImGui::Text("%s: Instigator Gameplay Tags:", TCHAR_TO_ANSI(*DamageModificationCategory));
				FGameplayTagContainer HitInstigatorGameplayTags = Entry.HitInstigatorTagsContainer;
				if (HitInstigatorGameplayTags.IsEmpty())
				{
					ImGui::TextColored(gameplaytagcolor,"%s Instigator Has No Gameplay Tags.", TCHAR_TO_ANSI(*DamageModificationCategory));
				}
				else
				{
					FString HitInstigatorGameplayTagsToolTip;					
					for (const FGameplayTag& Tag : Entry.HitInstigatorTagsContainer)
					{
						FString NewLine = FString::Printf(TEXT("%s\n"), *Tag.ToString());
						HitInstigatorGameplayTagsToolTip.Append(NewLine);
					}
					ImGui::TextColored(gameplaytagcolor,"%s", TCHAR_TO_ANSI(*HitInstigatorGameplayTagsToolTip));
				}
				ImGui::PopTextWrapPos();
					
				ImGui::TableNextColumn();
				colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
				ImGui::PushTextWrapPos(colEnd);
				ImGui::Text("%s: Target Actor: %s", TCHAR_TO_ANSI(*DamageModificationCategory), TCHAR_TO_ANSI(*Entry.HitTargetName));
				ImGui::Text("%s: Target Gameplay Tags:",TCHAR_TO_ANSI(*DamageModificationCategory));
				FGameplayTagContainer DamageTargetGameplayTags = Entry.HitTargetTagsContainer;
				if (DamageTargetGameplayTags.IsEmpty())
				{
					ImGui::TextColored(gameplaytagcolor,"No Target Gameplay Tags");
				}
				else
				{
					FString DamageTargetGameplayTagsToolTip;					
					for (const FGameplayTag& Tag : Entry.HitTargetTagsContainer)
					{
						FString NewLine = FString::Printf(TEXT("%s\n"), *Tag.ToString());
						DamageTargetGameplayTagsToolTip.Append(NewLine);
					}
					ImGui::TextColored(gameplaytagcolor,"%s", TCHAR_TO_ANSI(*DamageTargetGameplayTagsToolTip));
				}
				
				ImGui::PopTextWrapPos();	
				
				ImGui::TableNextColumn();
				colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
				ImGui::PushTextWrapPos(colEnd);
				ImGui::Text("%s: Context Gameplay Tags:\n", TCHAR_TO_ANSI(*DamageModificationCategory));
				
				FGameplayTagContainer ContextDataGameplayTags = Entry.HitContextTagsContainer;
				if (ContextDataGameplayTags.IsEmpty())
				{
					ImGui::TextColored(gameplaytagcolor,"No Context Gameplay Tags");
				}
				else
				{
					FString	ContextGameplayTagsToolTip;					
					for (const FGameplayTag& Tag : Entry.HitContextTagsContainer)
					{
						FString NewLine = FString::Printf(TEXT("%s\n"), *Tag.ToString());
						ContextGameplayTagsToolTip.Append(NewLine);
					}
					ImGui::TextColored(gameplaytagcolor, "%s", TCHAR_TO_ANSI(*ContextGameplayTagsToolTip));
				}
				
				FString CriticalCheckBoxID = FString::Printf(TEXT("Critical##%i"), Entry.DamageID);
				auto CritCheckBoxIdANSI = StringCast<ANSICHAR>(*CriticalCheckBoxID);
				ImGui::BeginDisabled(true);
				ImGui::Checkbox(CritCheckBoxIdANSI.Get(), &Entry.bIsCriticalHit);
				ImGui::EndDisabled();
				
				FString OverTimeEffectCheckBoxID = FString::Printf(TEXT("Over Time Effect##%i"), Entry.DamageID);
				auto OverTimeCheckBoxIdANSI = StringCast<ANSICHAR>(*OverTimeEffectCheckBoxID);
				ImGui::BeginDisabled(true);
				ImGui::Checkbox(OverTimeCheckBoxIdANSI.Get(), &Entry.bIsOverTimeEffect);
				ImGui::EndDisabled();
				
				ImGui::PopTextWrapPos();
				
				ImGui::TableNextColumn();
				colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
				ImGui::PushTextWrapPos(colEnd);
				ImGui::Text("Damage Modification Values:\n");
				
				ImVec4 modificationcolor = (Entry.bIsDamageEffect) ? red : green;
				FString RoundedBaseValueStr = FString::Printf(TEXT("%.3f"), Entry.BaseDamageValue);
				FString RoundedModifiedValueStr = FString::Printf(TEXT("%.3f"), Entry.ModifiedDamageValue);
				FString RoundedFinalValueStr = FString::Printf(TEXT("%.3f"), Entry.FinalDamageValue);
				ImGui::TextColored(modificationcolor, "Base Incoming Modification Value: %s", TCHAR_TO_ANSI(*RoundedBaseValueStr));
				ImGui::TextColored(modificationcolor, "Modified Value from Execution Class: %s", TCHAR_TO_ANSI(*RoundedModifiedValueStr));
				ImGui::TextColored(modificationcolor, "Final Damage Value: %s", TCHAR_TO_ANSI(*RoundedFinalValueStr));
				
				ImGui::Text("Contributing Attributes\n");
				FString ContributingAttributes;
				TArray<FString> AttributeNames;
				Entry.Attributes.GetKeys(AttributeNames);
				for (FString Attribute: AttributeNames)
				{
					FString NewLine = FString::Printf(TEXT("%s: %f\n"), *Attribute, *Entry.Attributes.Find(Attribute));
					ContributingAttributes.Append(NewLine);
				}
	
				ImGui::TextColored(attributecolor,"%s", TCHAR_TO_ANSI(*ContributingAttributes));
				ImGui::PopTextWrapPos();
				
				ImGui::TableNextColumn();
				colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
				ImGui::PushTextWrapPos(colEnd);
				ImGui::Text("Hit Result:");
				UScriptStruct* Struct = FHitResult::StaticStruct();
				FString Tooltip;
				for (TFieldIterator<FProperty> It(Struct); It; ++It)
				{
					FProperty* Prop = *It;
					FString Name = Prop->GetName();

					const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(&Entry.HitResult);

					FString Value;
					Prop->ExportTextItem_Direct(Value, ValuePtr, nullptr, nullptr, PPF_None);
					FString NewLine = FString::Printf(TEXT("%s: %s\n"), *Name, *Value);
					Tooltip.Append(NewLine);
				}
				
				ImGui::TextColored(white,"%s", TCHAR_TO_ANSI(*Tooltip));
				ImGui::PopTextWrapPos();
				
				ImGui::TableNextColumn();
				FString SimulateButtonID = FString::Printf(TEXT("Simulate %s ID: %i"), *DamageModificationCategory, Entry.DamageID);
				auto SimulateButtonIDANSI = StringCast<ANSICHAR>(*SimulateButtonID);
				if (ImGui::Button(SimulateButtonIDANSI.Get()))
				{
					DamagePipelineDebugSubsystem->SimulateDamageFromID(Entry.DamageID);
				}
			}
		}
		ImGui::EndTable();
		ImGui::PopStyleVar();
	}
	ImGui::End();
}

void FGASCDamageEventsPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}
