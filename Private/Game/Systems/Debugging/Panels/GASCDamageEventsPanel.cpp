// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/GASCDamageEventsPanel.h"
#include "imgui.h"
#include "Game/Systems/Damage/Debug/DamagePipelineDebugSubsystem.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

TWeakObjectPtr<APawn> FGASCDamageEventsPanel::SelectedPawn = nullptr;
EGASC_DamagePipelineType FGASCDamageEventsPanel::SelectedPipelineType = Healing;
TMap<uint32, bool> FGASCDamageEventsPanel::DrawHitResultStates;
TMap<uint32, ImVec4> FGASCDamageEventsPanel::DrawHitResultColors;
TMap<uint32, uint32> FGASCDamageEventsPanel::DrawHitResultRadii;
bool FGASCDamageEventsPanel::bFilterOutSimulated = false;
int FGASCDamageEventsPanel::DamageModContextFilter = 0;
int FGASCDamageEventsPanel::DamageModEventType = 0;
int FGASCDamageEventsPanel::DamageType = 0;

FGASCDamageEventsPanel::FGASCDamageEventsPanel()
{
}

FGASCDamageEventsPanel::~FGASCDamageEventsPanel()
{
	DrawHitResultStates.Empty();
	DrawHitResultColors.Empty();
}

void FGASCDamageEventsPanel::DrawDebugPanel(bool& bOpen)
{
    if (!bOpen)
        return;

    if (!ImGui::Begin("Damage Pipeline Damage Events", &bOpen))
    {
        ImGui::End();
        return;
    }

    // ==================== OPTIONS TABLE ====================
    FString OptionsTableId = TEXT("Options##Table");
    auto OptionsTableIdANSI = StringCast<ANSICHAR>(*OptionsTableId);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
    if (ImGui::BeginTable(OptionsTableIdANSI.Get(), 4,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_SizingStretchSame |
        ImGuiTableColumnFlags_WidthStretch |
        ImGuiTableFlags_BordersH))
    {
        ImGui::TableSetupColumn("Pawn Selection");
        ImGui::TableSetupColumn("Damage Modification Type");
        ImGui::TableSetupColumn("Event Type");
        ImGui::TableSetupColumn("Filters");
        ImGui::TableHeadersRow();

        // -------- Pawn Selection --------
        ImGui::TableNextColumn();
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
                    DrawHitResultStates.Empty();
                    DrawHitResultColors.Empty();
                }
            }
            ImGui::EndCombo();
        }

        // -------- Pipeline Type --------
        ImGui::TableNextColumn();
        FString EnumStr = StaticEnum<EGASC_DamagePipelineType>()->GetNameStringByValue((int64)SelectedPipelineType);
        const char* AnsiName = TCHAR_TO_ANSI(*EnumStr);

        if (ImGui::BeginCombo("Modification Type", AnsiName))
        {
            UEnum* EnumPtr = StaticEnum<EGASC_DamagePipelineType>();
            int32 EnumCount = EnumPtr->NumEnums() - 1;

            for (int32 i = 0; i < EnumCount; i++)
            {
                EGASC_DamagePipelineType EnumValue = (EGASC_DamagePipelineType)EnumPtr->GetValueByIndex(i);
                FString Name = EnumPtr->GetNameStringByIndex(i);
                const char* NameAnsi = TCHAR_TO_ANSI(*Name);

                bool bIsSelected = (SelectedPipelineType == EnumValue);

                if (ImGui::Selectable(NameAnsi, bIsSelected))
                {
                    SelectedPipelineType = EnumValue;
                    if (bIsSelected) ImGui::SetItemDefaultFocus();
                    DrawHitResultStates.Empty();
                    DrawHitResultColors.Empty();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TableNextColumn();
        ImGui::Combo("Event Type Filter", &DamageModEventType, "All\0EventReceived\0EventApplied");

        ImGui::TableNextColumn();
        if (SelectedPipelineType == Healing)
        {
            DamageType = 1;
            ImGui::BeginDisabled();
            ImGui::Combo("Damage Type Filter", &DamageType, "All\0Healing\0Physical\0Fire");
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::Combo("Damage Type Filter", &DamageType, "All\0Healing\0Physical\0Fire");
        }
    	
    	ImGui::Combo("Damage Context Filter", &DamageModContextFilter, "All\0CriticalOnly\0DamageOverTimeOnly\0ResistedDamageOnly");
        ImGui::Checkbox("Filter Simulated Damage", &bFilterOutSimulated);
        ImGui::EndTable();
    }

    ImGui::PopStyleVar(); // <-- BALANCED POP for OPTIONS TABLE


    if (!SelectedPawn.IsValid())
    {
        ImGui::End();
        return;
    }

    UDamagePipelineDebugSubsystem* DebugSys = SelectedPawn->GetWorld()->GetSubsystem<UDamagePipelineDebugSubsystem>();
    if (!DebugSys)
    {
        ImGui::End();
        return;
    }

    // ================= MAIN DAMAGE TABLE ====================
    FString TableId = FString::Printf(TEXT("%s##Table"), *SelectedPawn->GetName());
    auto TableIdANSI = StringCast<ANSICHAR>(*TableId);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));

    if (ImGui::BeginTable(TableIdANSI.Get(), 6,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_SizingStretchSame |
        ImGuiTableColumnFlags_WidthStretch |
        ImGuiTableFlags_BordersH))
    {
        ImGui::TableSetupColumn("Instigator Info");
        ImGui::TableSetupColumn("Target Info");
        ImGui::TableSetupColumn("Context Data");
        ImGui::TableSetupColumn("Modification Values");
        ImGui::TableSetupColumn("Hit Result");
        ImGui::TableSetupColumn("Simulate Damage");
        ImGui::TableHeadersRow();

        ImVec4 green(0, 1, 0, 1);
        ImVec4 red(1, 0, 0, 1);
        ImVec4 white(1, 1, 1, 1);
        ImVec4 tagColor(0, 1, 1, 1);
        ImVec4 attributeColor(1, 1, 0, 1);

        TArray<FDamageLogEntry> DamageEntries = DebugSys->GetDamageLogEntriesForActorID(SelectedPawn->GetUniqueID());

        for (FDamageLogEntry& Entry : DamageEntries)
        {
            if ((Entry.bIsDamageEffect && SelectedPipelineType != Damage) ||
                (!Entry.bIsDamageEffect && SelectedPipelineType != Healing))
                continue;

            AActor* Inst = DebugSys->GetActorFromID(Entry.DamageInstigatorID);
            AActor* Target = DebugSys->GetActorFromID(Entry.DamageTargetID);
            
            if (DamageModEventType == 1 && SelectedPawn != Target) continue;
            if (DamageModEventType == 2 && SelectedPawn != Inst) continue;
            if (Entry.HitContextTagsContainer.HasTagExact(DamageType_DebugSimulated) && bFilterOutSimulated) continue;
        	if (Entry.HitContextTagsContainer.HasTagExact(DamageType_Physical) && !(DamageType == 0 || DamageType == 2)) continue;
        	if (Entry.HitContextTagsContainer.HasTagExact(DamageType_Healing) && !(DamageType==0 || DamageType==1)) continue;
        	if (Entry.HitContextTagsContainer.HasTagExact(DamageType_Elemental_Fire) && !(DamageType==0 || DamageType==3)) continue;
        	switch (DamageModContextFilter)
        	{
        		case 0:
        			break;
        		case 1:
        			if (!Entry.bIsCriticalHit) continue;
        			break;
        		case 2:
        			if (!Entry.bIsOverTimeEffect) continue;
        			break;
        		case 3:
        			if (!Entry.bDamageResisted) continue;
        			break;
        	default:
        		break;
        	}

            FString Cat = Entry.bIsDamageEffect ? "Damage" : "Healing";

            // ================= COLUMN 0: Instigator Info =================
            ImGui::TableNextColumn();
            float colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
            ImGui::PushTextWrapPos(colEnd);

            ImGui::Text("%s: Instigator: %s", TCHAR_TO_ANSI(*Cat), TCHAR_TO_ANSI(*Entry.HitInstigatorName));
            ImGui::Text("%s: Tags:", TCHAR_TO_ANSI(*Cat));

            if (Entry.HitInstigatorTagsContainer.IsEmpty())
            {
                ImGui::TextColored(tagColor, "%s Instigator Has No Tags.", TCHAR_TO_ANSI(*Cat));
            }
            else
            {
                FString Tags;
                for (const auto& Tag : Entry.HitInstigatorTagsContainer)
                    Tags += Tag.ToString() + "\n";

                ImGui::TextColored(tagColor, "%s", TCHAR_TO_ANSI(*Tags));
            }
            ImGui::PopTextWrapPos();

            // ================= COLUMN 1: Target Info =================
            ImGui::TableNextColumn();
            colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
            ImGui::PushTextWrapPos(colEnd);

            ImGui::Text("%s: Target: %s", TCHAR_TO_ANSI(*Cat), TCHAR_TO_ANSI(*Entry.HitTargetName));
            ImGui::Text("%s: Tags:", TCHAR_TO_ANSI(*Cat));

            if (Entry.HitTargetTagsContainer.IsEmpty())
            {
                ImGui::TextColored(tagColor, "No Tags");
            }
            else
            {
                FString Tags;
                for (const auto& Tag : Entry.HitTargetTagsContainer)
                    Tags += Tag.ToString() + "\n";

                ImGui::TextColored(tagColor, "%s", TCHAR_TO_ANSI(*Tags));
            }
            ImGui::PopTextWrapPos();

            // ================= COLUMN 2: Context Tags =================
            ImGui::TableNextColumn();
            colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
            ImGui::PushTextWrapPos(colEnd);

            ImGui::Text("%s: Context Tags:", TCHAR_TO_ANSI(*Cat));

            if (Entry.HitContextTagsContainer.IsEmpty())
            {
                ImGui::TextColored(tagColor, "No Tags");
            }
            else
            {
                FString Tags;
                for (const auto& Tag : Entry.HitContextTagsContainer)
                    Tags += Tag.ToString() + "\n";

                ImGui::TextColored(tagColor, "%s", TCHAR_TO_ANSI(*Tags));
            }

            // Crit & OT flags
            ImGui::BeginDisabled();
            bool dummyCrit = Entry.bIsCriticalHit;
            bool dummyOT = Entry.bIsOverTimeEffect;
        	bool dummyResisted = Entry.bDamageResisted;
        	ImGui::PushID(Entry.DamageID);
            ImGui::Checkbox("Critical", &dummyCrit);
            ImGui::Checkbox("Over Time", &dummyOT);
        	ImGui::Checkbox("Resisted", &dummyResisted);
        	ImGui::PopID();
            ImGui::EndDisabled();

            ImGui::PopTextWrapPos();

            // ================= COLUMN 3: Modification =================
            ImGui::TableNextColumn();
            colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
            ImGui::PushTextWrapPos(colEnd);

            ImVec4 modColorV = Entry.bIsDamageEffect ? red : green;
            ImGui::TextColored(modColorV, "Base: %.3f", Entry.BaseDamageValue);
            ImGui::TextColored(modColorV, "Modified: %.3f", Entry.ModifiedDamageValue);
            ImGui::TextColored(modColorV, "Final: %.3f", Entry.FinalDamageValue);

            FString AttrText;
            TArray<FString> AttrNames;
            Entry.Attributes.GetKeys(AttrNames);
            for (const FString& N : AttrNames)
                AttrText += FString::Printf(TEXT("%s: %f\n"), *N, Entry.Attributes.FindChecked(N));

            ImGui::TextColored(attributeColor, "%s", TCHAR_TO_ANSI(*AttrText));

            ImGui::PopTextWrapPos();

            // ================= COLUMN 4: Hit Result =================
            ImGui::TableNextColumn();
            colEnd = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
            ImGui::PushTextWrapPos(colEnd);

            FString Tooltip;
            if (Entry.HitResult.bBlockingHit)
            {
                UScriptStruct* HRStruct = FHitResult::StaticStruct();
                for (TFieldIterator<FProperty> It(HRStruct); It; ++It)
                {
                    FProperty* Prop = *It;
                    FString Name = Prop->GetName();
                    const void* ValPtr = Prop->ContainerPtrToValuePtr<void>(&Entry.HitResult);

                    FString Export;
                    Prop->ExportTextItem_Direct(Export, ValPtr, nullptr, nullptr, PPF_None);
                    Tooltip += Name + TEXT(": ") + Export + TEXT("\n");
                }
            }
            else
            {
                Tooltip = "No Hit Result";
            }

            FString HRScrollID = FString::Printf(TEXT("HitResultData##%i"), Entry.DamageID);
            auto HRScrollAnsi = StringCast<ANSICHAR>(*HRScrollID);

            ImGui::BeginChild(HRScrollAnsi.Get(),
                ImVec2(ImGui::GetColumnWidth(), 200),
                true,
                ImGuiWindowFlags_HorizontalScrollbar);

            ImGui::TextColored(white, "%s", TCHAR_TO_ANSI(*Tooltip));
            ImGui::EndChild();

            // ---- Hit Result Options Table ----
            if (Entry.HitResult.bBlockingHit)
            {
                FString HitOptId = FString::Printf(TEXT("HitResultOptions##%i"), Entry.DamageID);
                auto HitOptAnsi = StringCast<ANSICHAR>(*HitOptId);

                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 8));

                if (ImGui::BeginTable(HitOptAnsi.Get(), 3,
                    ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_BordersH))
                {
                    ImGui::TableSetupColumn("Draw");
                    ImGui::TableSetupColumn("Color");
                    ImGui::TableSetupColumn("Radius");
                    ImGui::TableHeadersRow();

                    // Draw checkbox
                    ImGui::TableNextColumn();
                    bool& bDrawHit = DrawHitResultStates.FindOrAdd(Entry.DamageID);
                    ImGui::Checkbox("##DrawHR", &bDrawHit);

                    // Color picker
                    ImGui::TableNextColumn();
                    ImVec4& HRColor = DrawHitResultColors.FindOrAdd(Entry.DamageID, ImVec4(1, 0, 0, 1));

                    ImGui::PushID(Entry.DamageID);
                    ImGui::ColorEdit4("##ColorPicker", (float*)&HRColor,
                        ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_PickerHueWheel |
                        ImGuiColorEditFlags_DisplayRGB);
                    ImGui::PopID();

                    // Radius slider
                    ImGui::TableNextColumn();
                    uint32& DebugRadius = DrawHitResultRadii.FindOrAdd(Entry.DamageID, 20);
                    ImGui::SliderInt("##DrawRadius", (int*)&DebugRadius, 5, 100);

                    // Draw debug shape
                    if (bDrawHit)
                    {
                        FColor SphereColor(
                            (uint8)(HRColor.x * 255),
                            (uint8)(HRColor.y * 255),
                            (uint8)(HRColor.z * 255),
                            (uint8)(HRColor.w * 255));

                        DrawDebugSphere(
                            SelectedPawn->GetWorld(),
                            Entry.HitResult.Location,
                            DebugRadius,
                            16,
                            SphereColor);
                    }

                    ImGui::EndTable();
                }

                ImGui::PopStyleVar();  // <-- FIXED: Missing PopStyleVar for HitResultOptions
            }

            ImGui::PopTextWrapPos();

            // ================= COLUMN 5: Simulate Damage =================
            ImGui::TableNextColumn();
            FString SimID = FString::Printf(TEXT("Simulate %s ID: %i"), *Cat, Entry.DamageID);
            auto SimIDAnsi = StringCast<ANSICHAR>(*SimID);

            if (ImGui::Button(SimIDAnsi.Get()))
            {
                DebugSys->SimulateDamageFromID(Entry.DamageID);
            }
        }
        ImGui::EndTable();
    }

    ImGui::PopStyleVar(); // end main table padding
    ImGui::End();
}


void FGASCDamageEventsPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}
