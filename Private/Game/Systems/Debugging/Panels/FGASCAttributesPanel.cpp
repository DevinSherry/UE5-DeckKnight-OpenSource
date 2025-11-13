// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Debugging/Panels/FGASCAttributesPanel.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "imgui.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASC_CardResourcesAttributeSet.h"

TWeakObjectPtr<APawn> FGASCAttributesPanel::SelectedPawn = nullptr;
TWeakObjectPtr<UAbilitySystemComponent> FGASCAttributesPanel::SelectedASC = nullptr;
static TArray<FGameplayAttribute> AttributesToMonitor;
static TMap<FGameplayAttribute, bool> AttributeHistoryWindows;

FGASCAttributesPanel::FGASCAttributesPanel()
{
}

FGASCAttributesPanel::~FGASCAttributesPanel()
{
	AttributesToMonitor.Empty();
	AttributeHistoryWindows.Empty();
	SelectedPawn = nullptr;
	SelectedASC = nullptr;
}

void FGASCAttributesPanel::DrawDebugPanel(bool& bOpen)
{
    if (!bOpen)
    {
    	AttributesToMonitor.Empty();
    	return;
    }

    if (!ImGui::Begin("Gameplay Attributes", &bOpen))
    {
        ImGui::End();
        return;
    }

    // Pawn selection combo
    if (ImGui::BeginCombo("Pawns", SelectedPawn.IsValid() ? TCHAR_TO_ANSI(*SelectedPawn->GetName()) : "None"))
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

                UAbilitySystemComponent* ASC = SelectedPawn->FindComponentByClass<UAbilitySystemComponent>();
                if (!ASC && SelectedPawn->IsPlayerControlled())
                {
                    ASC = SelectedPawn->GetPlayerState()->FindComponentByClass<UAbilitySystemComponent>();
                }

                SelectedASC = ASC;
                InitializeAbilitySystemComponent(ASC);
            }
        }
        ImGui::EndCombo();
    }

    // Attributes table
    if (SelectedASC.IsValid())
    {
        for (UAttributeSet* AttrSet : SelectedASC->GetSpawnedAttributes())
        {
            if (!AttrSet) continue;

            UClass* AttrClass = AttrSet->GetClass();
            if (!AttrClass) continue;

            auto AttributeNameANSI = StringCast<ANSICHAR>(*AttrClass->GetName());
            if (!ImGui::CollapsingHeader(AttributeNameANSI.Get())) continue;

            FString TableId = FString::Printf(TEXT("%s##Table"), *AttrClass->GetName());
            auto TableIdANSI = StringCast<ANSICHAR>(*TableId);

            if (ImGui::BeginTable(TableIdANSI.Get(), 5, ImGuiTableFlags_SizingFixedFit |
                                                         ImGuiTableFlags_Resizable |
                                                         ImGuiTableFlags_BordersV |
                                                         ImGuiTableFlags_Reorderable |
                                                         ImGuiTableFlags_Hideable |
                                                         ImGuiTableFlags_SizingStretchSame))
            {
                ImGui::TableSetupColumn("Attribute");
                ImGui::TableSetupColumn("Base");
                ImGui::TableSetupColumn("Current");
                ImGui::TableSetupColumn("Delta");
                ImGui::TableSetupColumn("Modify Attribute");
                ImGui::TableHeadersRow();

                for (TFieldIterator<FProperty> It(AttrClass); It; ++It)
                {
                    FStructProperty* StructProp = CastField<FStructProperty>(*It);
                    if (!StructProp || StructProp->Struct != FGameplayAttributeData::StaticStruct())
                        continue;

                    FGameplayAttribute Attribute(StructProp);
                    FString AttrName = StructProp->GetName();

                    float CurrentValue = SelectedASC->GetNumericAttribute(Attribute);
                    float BaseValue = SelectedASC->GetNumericAttributeBase(Attribute);

                    ImVec4 green = ImVec4(0, 1, 0, 1);
                    ImVec4 red   = ImVec4(1, 0, 0, 1);
                    ImVec4 white = ImVec4(1, 1, 1, 1);

                    ImVec4 color = (CurrentValue > BaseValue) ? green : (CurrentValue < BaseValue ? red : white);

                    ImGui::PushID(TCHAR_TO_ANSI(*AttrName));
                    ImGui::TableNextRow();

                    // Attribute Name
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", TCHAR_TO_ANSI(*AttrName));

                    // Context menu
                    if (ImGui::BeginPopupContextItem("Attribute Context Menu"))
                    {
                        if (ImGui::MenuItem("Monitor Attribute History"))
                        {
                            AttributesToMonitor.Add(Attribute);
                        }
                        ImGui::EndPopup();
                    }

                    // Base value
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", BaseValue);

                    // Current value
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%.2f", CurrentValue);

                    // Delta
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%.2f", CurrentValue - BaseValue);

                    // Input for modifying attribute
                    ImGui::TableNextColumn();
                    if (SelectedASC.IsValid() && ImGui::InputFloat("", &CurrentValue))
                    {
                        SelectedASC->SetNumericAttributeBase(Attribute, CurrentValue);
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
    }

    ImGui::End();

    // Attribute history windows
    for (FGameplayAttribute AttributeToCheck : AttributesToMonitor)
    {
	    FGameplayAttribute Attribute = AttributeToCheck;

    	bool* bWindowOpenPtr = AttributeHistoryWindows.Find(Attribute);
    	if (!bWindowOpenPtr)
    	{
    		AttributeHistoryWindows.Add(Attribute, true);
    		bWindowOpenPtr = AttributeHistoryWindows.Find(Attribute);
    	}

    	bool& bWindowOpen = *bWindowOpenPtr;
    	FString WindowTitle = FString::Printf(TEXT("History: %s"), *Attribute.GetName());

    	if (!ImGui::Begin(TCHAR_TO_ANSI(*WindowTitle), &bWindowOpen))
    	{
    		ImGui::End();
    		// Remove closed windows from tracking
    		if (!bWindowOpen)
    		{
    			AttributesToMonitor.Remove(AttributeToCheck);
    			AttributeHistoryWindows.Remove(AttributeToCheck);
    		}
    		continue;
    	}

    	TArray<FAttributeHistoryEntry> AttributeHistoryEntries;
    	if (SelectedPawn.IsValid())
    	{
    		if (UGASC_DebugSubsystem* DebugSubsystem = SelectedPawn->GetWorld()->GetGameInstance()->GetSubsystem<UGASC_DebugSubsystem>())
    		{
    			if (const TArray<FAttributeHistoryEntry>* History = DebugSubsystem->AttributeHistory.Find(SelectedPawn.Get()))
    			{
    				for (const FAttributeHistoryEntry& Entry : *History)
    				{
    					if (Entry.AttributeName == Attribute.GetName())
    						AttributeHistoryEntries.Add(Entry);
    				}
    			}
    		}
    	}

    	// Draw table
    	if (ImGui::BeginTable("AttrHistoryTable", 6,
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_Resizable))
    	{
    		ImGui::TableSetupScrollFreeze(1, 1);
    		ImGui::TableSetupColumn("New");
    		ImGui::TableSetupColumn("Old");
    		ImGui::TableSetupColumn("Delta");
    		ImGui::TableSetupColumn("Instigator");
    		ImGui::TableSetupColumn("Effect");
    		ImGui::TableSetupColumn("Execution Class");
    		ImGui::TableHeadersRow();

    		if (AttributeHistoryEntries.Num() == 0)
    		{
    			for (int j = 0; j < 6; ++j)
    			{
    				ImGui::TableNextColumn();
    				ImGui::Text("None");
    			}
    		}
    		else
    		{
    			for (const FAttributeHistoryEntry& Entry : AttributeHistoryEntries)
    			{
    				if (Entry.OldValue == Entry.NewValue)
    					continue;

    				ImVec4 green = ImVec4(0, 1, 0, 1);
    				ImVec4 red   = ImVec4(1, 0, 0, 1);
    				ImVec4 white = ImVec4(1, 1, 1, 1);

    				ImVec4 color = (Entry.NewValue > Entry.OldValue) ? green : (Entry.NewValue < Entry.OldValue ? red : white);

    				ImGui::TableNextRow();
    				ImGui::TableNextColumn(); ImGui::Text("%.2f", Entry.NewValue);
    				ImGui::TableNextColumn(); ImGui::Text("%.2f", Entry.OldValue);
    				ImGui::TableNextColumn(); ImGui::TextColored(color, "%.2f", Entry.NewValue - Entry.OldValue);
    				ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.InstigatorName));
    				ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.EffectName));
    				ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.ExecutionClassName));
    			}
    		}

    		ImGui::EndTable();
    	}

    	ImGui::End();

    	// If the user closed the window, remove it from tracking
    	if (!bWindowOpen)
    	{
    		AttributesToMonitor.Remove(AttributeToCheck);
    		AttributeHistoryWindows.Remove(AttributeToCheck);
    	}
    }
}

void FGASCAttributesPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}

void FGASCAttributesPanel::InitializeAbilitySystemComponent(UAbilitySystemComponent* ASC)
{
}
