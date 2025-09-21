// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Debugging/Panels/FGASCAttributesPanel.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameFramework/PlayerState.h"
#include "imgui.h"
#include "Game/GameplayAbilitySystem/AttributeSets/GASC_CardResourcesAttributeSet.h"

TWeakObjectPtr<APawn> FGASCAttributesPanel::SelectedPawn = nullptr;
TWeakObjectPtr<UAbilitySystemComponent> FGASCAttributesPanel::SelectedASC = nullptr;
static TArray<FGameplayAttribute> AttributesToMonitor;
TArray<FOnAttributeChangeData> FGASCAttributesPanel::ChangedHistory;
TMap<const FProperty*, TArray<FAttributeHistoryEntry>> FGASCAttributesPanel::AttributeHistories;
static bool bAttributeMonitorOpen = true;

FGASCAttributesPanel::FGASCAttributesPanel()
{
}

FGASCAttributesPanel::~FGASCAttributesPanel()
{
}

void FGASCAttributesPanel::DrawDebugPanel(bool& bOpen)
{
		if (ImGui::Begin("Gameplay Attributes", &bOpen))
		{
			if (ImGui::BeginCombo("Pawns",
						(SelectedPawn.IsValid() ? TCHAR_TO_ANSI(*SelectedPawn->GetName()) : "None")))
			{
				for (TWeakObjectPtr<APawn> It : CachedPawns)//TActorIterator<APawn> It(GetWorld()); It; ++It
				{
					APawn* Pawn = It.Get();
					if (!IsValid(Pawn)) continue;

					bool bIsSelected = (Pawn == SelectedPawn.Get());
					FString PawnName = Pawn->GetName();

					if (ImGui::Selectable(TCHAR_TO_ANSI(*PawnName), bIsSelected))
					{
						SelectedPawn = Pawn;
						if (bIsSelected)
							ImGui::SetItemDefaultFocus();
						UAbilitySystemComponent* ASC = SelectedPawn->FindComponentByClass<UAbilitySystemComponent>();
						if (!ASC)
						{
							if (SelectedPawn->IsPlayerControlled())
							{
								ASC = SelectedPawn->GetPlayerState()->FindComponentByClass<UAbilitySystemComponent>();
							}
						}
						SelectedASC = ASC;
						InitializeAbilitySystemComponent(ASC);
					}
				}
				ImGui::EndCombo();
			}

			if (SelectedPawn.IsValid())
			{
				UAbilitySystemComponent* ASC = SelectedPawn->FindComponentByClass<UAbilitySystemComponent>();
				if (!ASC)
				{
					if (SelectedPawn->IsPlayerControlled())
					{
						ASC = SelectedPawn->GetPlayerState()->FindComponentByClass<UAbilitySystemComponent>();
					}
				}
				SelectedASC = ASC;
				if (SelectedASC.IsValid())
				{
					for (UAttributeSet* AttrSet : SelectedASC->GetSpawnedAttributes())
					{
						UClass* AttrClass = AttrSet->GetClass();
						auto AttributeNameANSI = StringCast<ANSICHAR>(*AttrClass->GetName());
						if (ImGui::CollapsingHeader(AttributeNameANSI.Get()))
						{
							FString TableId = FString::Printf(TEXT("%s##Table"), *AttrClass->GetName());
							auto TableIdANSI = StringCast<ANSICHAR>(*TableId);
							if (ImGui::BeginTable(TableIdANSI.Get(), 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingStretchSame))
							{
								ImGui::TableSetupColumn("Attribute");
								ImGui::TableSetupColumn("Base");
								ImGui::TableSetupColumn("Current");
								ImGui::TableSetupColumn("Delta");
								ImGui::TableSetupColumn("Modify Attribute");
								ImGui::TableHeadersRow();

								for (TFieldIterator<FProperty> It(AttrClass); It; ++It)
								{
									if (FStructProperty* StructProp = CastField<FStructProperty>(*It))
									{
										if (StructProp->Struct == FGameplayAttributeData::StaticStruct())
										{
											ImVec4 green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
											ImVec4 red   = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
											ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
											
											// Convert to attribute
											FGameplayAttribute Attribute(StructProp);

											float CurrentValue = SelectedASC->GetNumericAttribute(Attribute);
											float BaseValue = SelectedASC->GetNumericAttributeBase(Attribute);
											FString Name = StructProp->GetName();
											ImGui::PushID(TCHAR_TO_ANSI(*Name));

											ImGui::TableNextRow();
											
											ImGui::TableNextColumn();
											ImGui::Text("%s", TCHAR_TO_ANSI(*Name));
											
											if (ImGui::BeginPopupContextItem("Attribute Context Menu"))
											{
												if (ImGui::MenuItem("Monitor Attribute History"))
												{
													AttributesToMonitor.Add(Attribute);
												}

												ImGui::EndPopup();
											}

											ImGui::TableNextColumn();
											ImGui::Text("%.2f", BaseValue);

											ImVec4 color = white;
											if (CurrentValue > BaseValue) color = green;
											else if (CurrentValue < BaseValue) color = red;
											
											ImGui::TableNextColumn();
											ImGui::TextColored(color,"%.2f", CurrentValue);

											ImGui::TableNextColumn();
											ImGui::TextColored(color,"%.2f", CurrentValue - BaseValue);

											ImGui::TableNextColumn();
											if (ImGui::InputFloat("", &CurrentValue))
											{
												SelectedASC->SetNumericAttributeBase(Attribute, CurrentValue);
											}
											
											ImGui::PopID();
										}
									}
								}
								ImGui::EndTable();
							}
						}
					}
				}
			}
		}
		ImGui::End();

		for (FGameplayAttribute Attribute : AttributesToMonitor)
		{
			FString WindowTitle = FString::Printf(TEXT("History: %s"), *Attribute.GetName());
			/*
			if (bAttributeMonitorOpen)
			{
				
			}
			*/
			if (ImGui::Begin(TCHAR_TO_ANSI(*WindowTitle), &bAttributeMonitorOpen))
			{
				const FProperty* Prop = Attribute.GetUProperty();
				if (const TArray<FAttributeHistoryEntry>* History = AttributeHistories.Find(Prop))
				{
					if (ImGui::BeginTable("AttrHistoryTable", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
						ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
					{
						ImGui::TableSetupScrollFreeze(1,1);
						
						ImGui::TableSetupColumn("New");
						ImGui::TableSetupColumn("Old");
						ImGui::TableSetupColumn("Delta");
						ImGui::TableSetupColumn("Instigator");
						ImGui::TableSetupColumn("Effect");
						ImGui::TableSetupColumn("Execution Class");
						ImGui::TableHeadersRow();

						if (History->Num() == 0)
						{
							ImGui::TableNextColumn(); ImGui::Text("None");
							ImGui::TableNextColumn(); ImGui::Text("None");
							ImGui::TableNextColumn(); ImGui::Text("None");
							ImGui::TableNextColumn(); ImGui::Text("None");
							ImGui::TableNextColumn(); ImGui::Text("None");
							ImGui::TableNextColumn(); ImGui::Text("None");
							return;
						}
					
						for (const FAttributeHistoryEntry& Entry : *History)
						{
							if (Entry.OldValue == Entry.NewValue)
							{
								break;
							}
							ImGui::TableNextRow();
							
							ImGui::TableNextColumn(); ImGui::Text("%.2f", Entry.NewValue);
							ImGui::TableNextColumn(); ImGui::Text("%.2f", Entry.OldValue);

							float MaxCardEnergyXP = 0.0f;
							if (SelectedASC.IsValid())
							{
								MaxCardEnergyXP = SelectedASC->GetNumericAttributeBase(UGASC_CardResourcesAttributeSet::GetMaximumCardEnergyXPAttribute());
							}
							if ((Entry.NewValue - Entry.OldValue) < 0.0f)
							{
								ImGui::TableNextColumn(); ImGui::Text("%.2f", (MaxCardEnergyXP) + (Entry.NewValue - Entry.OldValue));
							}
							else
							{
								ImGui::TableNextColumn(); ImGui::Text("%.2f", (Entry.NewValue - Entry.OldValue));
							}
							ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.InstigatorName));
							ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.EffectName));
							ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*Entry.ExecutionClassName));
						}

						ImGui::EndTable();
					}
				}
			}
			ImGui::End();
			
			// Handle close button
			if (!bOpen)
			{
				AttributesToMonitor.Empty();
				AttributeHistories.Empty();
				break; // break because we modified the set while iterating
			}
		}
}

void FGASCAttributesPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}

void FGASCAttributesPanel::InitializeAbilitySystemComponent(UAbilitySystemComponent* ASC)
{
	const TArray<UAttributeSet*> AttrSets = SelectedASC->GetSpawnedAttributes();

	for (UAttributeSet* AttrSet : AttrSets)
	{
	    UClass* AttrClass = AttrSet->GetClass();
	    for (TFieldIterator<FProperty> It(AttrClass); It; ++It)
	    {
	        if (FStructProperty* StructProp = CastField<FStructProperty>(*It))
	        {
	            if (StructProp->Struct == FGameplayAttributeData::StaticStruct())
	            {
	                // Capture attribute name as string to ensure it stays valid
	                FGameplayAttribute Attribute(StructProp);
	                FString AttributeName = Attribute.GetName();

	                UE_LOG(LogTemp, Warning, TEXT("Registering delegate for attribute: %s"), *AttributeName);

	                // Register delegate
	                ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddLambda(
	                    [this, StructProp, AttributeName](const FOnAttributeChangeData& Data)
	                    {
	                        // Default values
	                        FString InstigatorName = TEXT("Unknown");
	                        FString EffectName = TEXT("Unknown");
	                        FString ExecutionClassName = TEXT("Unknown");

	                        if (Data.GEModData)
	                        {
	                            const FGameplayEffectSpec& Spec = Data.GEModData->EffectSpec;

	                            // Effect definition
	                            if (Spec.Def)
	                            {
	                                EffectName = Spec.Def->GetFullName();

	                                if (Spec.Def->Executions.Num() > 0)
	                                {
	                                    ExecutionClassName = Spec.Def->Executions[0].CalculationClass->GetFullName();
	                                }
	                            }

	                            // Context (instigator / causer)
	                            const FGameplayEffectContextHandle& Ctx = Spec.GetContext();
	                            if (Ctx.IsValid())
	                            {
	                                if (AActor* Inst = Ctx.GetOriginalInstigator())
	                                {
	                                    InstigatorName = Inst->GetFullName();
	                                }
	                                else if (AActor* Causer = Ctx.GetEffectCauser())
	                                {
	                                    InstigatorName = Causer->GetFullName();
	                                }
	                            }
	                        }

	                        // Save to history using UProperty* as key
	                        AttributeHistories.FindOrAdd(static_cast<const FProperty*>(StructProp)).Add(
	                            FAttributeHistoryEntry(
	                                AttributeName,       // Use captured string
	                                Data.OldValue,
	                                Data.NewValue,
	                                InstigatorName,
	                                EffectName,
	                                ExecutionClassName
	                            )
	                        );
	                    });
	            }
	        }
	    }
	}
}
