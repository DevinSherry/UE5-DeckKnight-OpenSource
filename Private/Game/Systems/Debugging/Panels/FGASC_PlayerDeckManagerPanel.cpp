// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASC_PlayerDeckManagerPanel.h"
#include "imgui.h"
#include "ImGuiModule.h" // or your plugin’s header
#include "Game/Character/Player/GASCoursePlayerState.h"
#include "GASCourse/GASCourseCharacter.h"

TArray<FAssetData> FGASC_PlayerDeckManagerPanel::CachedCardDataAssets;
TSoftObjectPtr<UCardDataAsset> FGASC_PlayerDeckManagerPanel::SelectedCardAsset;
UDeckManagerComponent* FGASC_PlayerDeckManagerPanel::DeckManagerComponent = nullptr;
TMap<TWeakObjectPtr<UTexture>, FImGuiTextureHandle> FGASC_PlayerDeckManagerPanel::TextureHandles;
TMap<TWeakObjectPtr<UBaseCardGameplayAbilitySet>, float> FGASC_PlayerDeckManagerPanel::CardLevelOverrides;
bool FGASC_PlayerDeckManagerPanel::bFilterForEmptyOrMissingCardData = false;
bool FGASC_PlayerDeckManagerPanel::bDebugIgnoreCardCosts = false;
int FGASC_PlayerDeckManagerPanel::MissingCardInfoFilter = 0;

FImGuiTextureHandle FGASC_PlayerDeckManagerPanel::GetOrRegisterTexture(UTexture* Texture)
{
	check(Texture);

	if (FImGuiTextureHandle* Found = TextureHandles.Find(Texture))
	{
		return *Found;
	}

	// Stable unique name
	const FName Name(*Texture->GetPathName());

	FImGuiTextureHandle Handle = FImGuiModule::Get().RegisterTexture(Name, Texture, /*bMakeUnique=*/false);
	TextureHandles.Add(Texture, Handle);
	return Handle;
}

float FGASC_PlayerDeckManagerPanel::GetOrAddCardLevelOverride(UBaseCardGameplayAbilitySet* CardSet)
{
	check(CardSet);
	if (CardLevelOverrides.Contains(CardSet))
	{
		return *CardLevelOverrides.Find(CardSet);
	}
	CardLevelOverrides.Add(CardSet, 1.0f);
	return 1.0f;
}

FGASC_PlayerDeckManagerPanel::FGASC_PlayerDeckManagerPanel()
{

}

FGASC_PlayerDeckManagerPanel::~FGASC_PlayerDeckManagerPanel()
{
	DeckManagerComponent = nullptr;
	CachedCardDataAssets.Empty();
	SelectedCardAsset = nullptr;
	
	if (FImGuiModule::IsAvailable())
	{
		FImGuiModule& Module = FImGuiModule::Get();
		for (auto& Kvp : TextureHandles)
		{
			Module.ReleaseTexture(Kvp.Value);
		}
		TextureHandles.Empty();
	}
	CardLevelOverrides.Empty();
}

void FGASC_PlayerDeckManagerPanel::DrawDebugPanel(bool& bOpen)
{
	if (!bOpen)
		return;

	if (!ImGui::Begin("Deck Management", &bOpen))
	{
		ImGui::End();
		return;
	}
	
	if (CachedCardDataAssets.Num() == 0)
	{
		CachedCardDataAssets = CacheCardAssets();
	}
	if (!DeckManagerComponent)
	{
		for (const TWeakObjectPtr<APawn>& PawnPtr : CachedPawns)
		{
			APawn* Pawn = PawnPtr.Get();
			if (!Pawn || !Pawn->GetWorld())
			{
				continue;
			}

			const AGASCourseCharacter* Character = Cast<AGASCourseCharacter>(Pawn);
			if (!Character)
			{
				continue;
			}

			AGASCoursePlayerState* PlayerState = Pawn->GetPlayerState<AGASCoursePlayerState>();
			if (!PlayerState)
			{
				continue;
			}

			DeckManagerComponent = PlayerState->GetDeckManagerComponent();
			bDebugIgnoreCardCosts = DeckManagerComponent->DebugIgnoreCardCostEnabled();
		}
	}
	
	if (!World)
	{
		for (TWeakObjectPtr<APawn> Pawn : CachedPawns)
		{
			if (Pawn.Get() && Pawn.Get()->GetWorld())
			{
				World = Pawn.Get()->GetWorld();
			}
		}
	}
	
	    // ==================== OPTIONS TABLE ====================
    FString OptionsTableId = TEXT("Options##Table");
    auto OptionsTableIdANSI = StringCast<ANSICHAR>(*OptionsTableId);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
    if (ImGui::BeginTable(OptionsTableIdANSI.Get(), 2,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_SizingStretchSame |
        ImGuiTableColumnFlags_WidthStretch |
        ImGuiTableFlags_BordersH))
    {
    	ImGui::TableSetupColumn("Debug Options");
    	ImGui::TableSetupColumn("Search & Filters");
        ImGui::TableHeadersRow();

        // -------- Debug Selection --------
        ImGui::TableNextColumn();
    	ImGui::Text("Debug Options");
    	if (ImGui::Checkbox("Ignore Card Costs", &bDebugIgnoreCardCosts))
    	{
    		if (DeckManagerComponent)
    		{
    			DeckManagerComponent->SetDebugIgnoreCardCost(bDebugIgnoreCardCosts);
    		}
    	}

        // -------- Search & Filters --------
        ImGui::TableNextColumn();
    	//ImGui::Checkbox("Filter for Empty or Missing Card Data", &bFilterForEmptyOrMissingCardData);
    	ImGui::Combo("Missing Card Info Filter", &MissingCardInfoFilter, "ShowAll\0MissingCardName\0MissingCardDescription\0MissingCardType\0MissingCardIcon\0MissingAbilitySet\0");
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
	
	FString AllCardsTableID = TEXT("All Cards##Table");
	auto AllCardsTableIdANSI = StringCast<ANSICHAR>(*AllCardsTableID);
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
	if (ImGui::BeginTable(AllCardsTableIdANSI.Get(), 4, 
		ImGuiTableFlags_SizingFixedFit |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Card Directory Data");
		ImGui::TableSetupColumn("Card Info");
		ImGui::TableSetupColumn("Card Ability Info");
		ImGui::TableSetupColumn("Actions");
		ImGui::TableHeadersRow();

		ImVec4 green = ImVec4(0, 1, 0, 1);
		ImVec4 red   = ImVec4(1, 0, 0, 1);
		ImVec4 white = ImVec4(1, 1, 1, 1);
		ImVec4 yellow = ImVec4(1, 1, 0, 1);
		
		for (int32 i = 0; i < CachedCardDataAssets.Num(); ++i)
		{
			const FAssetData& AssetData = CachedCardDataAssets[i];
			if (!AssetData.IsValid()) continue;
			
			SelectedCardAsset = TSoftObjectPtr<UCardDataAsset>(AssetData.ToSoftObjectPath());
			if (UCardDataAsset* LoadedCard = SelectedCardAsset.LoadSynchronous())
			{

				bool bShouldBeFiltered = LoadedCard->CardData.CardInfo.CardName.IsEmpty() || LoadedCard->CardData.CardInfo.CardDescription.IsEmpty()
				|| LoadedCard->CardData.CardInfo.CardType == EmptyCard || LoadedCard->CardData.CardInfo.CardIcon.IsNull();
				
				if (bFilterForEmptyOrMissingCardData && !bShouldBeFiltered)
					continue;
				
				switch (MissingCardInfoFilter)
				{
				case 0:
					break;
				case 1:
					if (!LoadedCard->CardData.CardInfo.CardName.IsEmpty()) continue;
					break;
				case 2:
					if (!LoadedCard->CardData.CardInfo.CardDescription.IsEmpty()) continue;
					break;
				case 3:
					if (LoadedCard->CardData.CardInfo.CardType != EmptyCard) continue;
					break;
				case 4:
					if (!LoadedCard->CardData.CardInfo.CardIcon.IsNull()) continue;
					break;
				case 5:
					if (LoadedCard->CardData.CardAbilitySet->IsValidLowLevel()) continue;
				default:
					break;
				}
					
				
				ImGui::PushID(i); // ✅ stable unique ID
				ImGui::TableNextRow();
			
				ImGui::TableNextColumn();
				
				const FString CardDataAssetName = AssetData.AssetName.ToString();
				const FString CardDataDirectoryPath = AssetData.PackagePath.ToString();
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardDataAssetName));
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardDataDirectoryPath));
				
#if WITH_EDITOR
				OpenAsset(LoadedCard);
#endif
				
				ImGui::TableNextColumn();
				
				ImVec4 CardNameTextColor = LoadedCard->CardData.CardInfo.CardName.IsEmpty() ? red : yellow;
				const FString CardName = LoadedCard->CardData.CardInfo.CardName.ToString().IsEmpty() ? "Missing Card Name!" :
				LoadedCard->CardData.CardInfo.CardName.ToString();
				
				ImGui::Text("Card Name:");
				ImGui::SameLine();
				ImGui::TextColored(CardNameTextColor, "%s", TCHAR_TO_ANSI(*CardName));
				ImGui::Separator();
				
				ImVec4 CardDescriptionTextColor = LoadedCard->CardData.CardInfo.CardDescription.ToString().IsEmpty() ? red : yellow;
				const FString CardDescription = LoadedCard->CardData.CardInfo.CardDescription.ToString().IsEmpty() ? "Missing Card Description!" :
				LoadedCard->CardData.CardInfo.CardDescription.ToString();
				ImGui::Text("Card Description:");
				ImGui::SameLine();
				ImGui::TextColored(CardDescriptionTextColor, "%s", TCHAR_TO_ANSI(*CardDescription));
				ImGui::Separator();
				
				ImVec4 CardTypeTextColor = LoadedCard->CardData.CardInfo.CardType == ECardType::EmptyCard ? red : yellow;
				const FString CardType =
				StaticEnum<ECardType>()->GetNameStringByValue((int64)LoadedCard->CardData.CardInfo.CardType);
				ImGui::Text("Card Slot Type:");
				ImGui::SameLine();
				ImGui::TextColored(CardTypeTextColor, "%s", TCHAR_TO_ANSI(*CardType));
				ImGui::Separator();
				
				static UTexture2D* DefaultIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
				UTexture2D* IconToDraw = nullptr;
				
				TSoftObjectPtr<UTexture2D> CardIcon = TSoftObjectPtr<UTexture2D>(LoadedCard->CardData.CardInfo.CardIcon.ToSoftObjectPath());
				if (UTexture2D* LoadedCardIcon = CardIcon.LoadSynchronous())
				{
					IconToDraw = LoadedCardIcon;
				}
				else
				{
					IconToDraw = DefaultIcon;
				}
				
				if (IconToDraw)
				{
					const FImGuiTextureHandle Handle = GetOrRegisterTexture(IconToDraw);

					// Most likely: you can pass Handle directly as ImTextureID:
					ImGui::Image(Handle, ImVec2(64, 64));
#if WITH_EDITOR
					if (ImGui::IsItemClicked())
					{
						OpenAsset(IconToDraw, true);
					}
#endif
				}
				
				ImGui::TableNextColumn();
				
				ImGui::Text("Ability Set:");
				if (UBaseCardGameplayAbilitySet* CardAbilitySet = LoadedCard->CardData.CardAbilitySet)
				{
					ImGui::Text("Granted Gameplay Abilities:");
					if (CardAbilitySet->GrantedGameplayAbilities.Num() == 0)
					{
						ImGui::Text("%s has no granted gameplay abilities.",TCHAR_TO_ANSI(*CardAbilitySet->GetName()));
					}
					for (const FGASCourseCardAbilitySet_GameplayAbility& AbilityEntry : CardAbilitySet->GrantedGameplayAbilities)
					{
						TSoftClassPtr<UGASCourseGameplayAbility> SoftAbilityRef = TSoftClassPtr<UGASCourseGameplayAbility>(AbilityEntry.GameplayAbility.ToSoftObjectPath());
						if (UGASCourseGameplayAbility* LoadedAbility = SoftAbilityRef.LoadSynchronous()->GetDefaultObject<UGASCourseGameplayAbility>())
						{
							const FString AbilityName = LoadedAbility->GetName();
							ImGui::Text("%s", TCHAR_TO_ANSI(*AbilityName));
#if WITH_EDITOR
							OpenBlueprintAsset(Cast<UBlueprint>(LoadedAbility->GetClass()->ClassGeneratedBy));
#endif
							const FString AbilityType =
							StaticEnum<EGASCourseAbilitySlotType>()->GetNameStringByValue((int64)AbilityEntry.AbilitySlotType);
							ImGui::Text("Ability Slot Type: %s", TCHAR_TO_ANSI(*AbilityType));
						}
					}
					
					ImGui::Separator();
					
					ImGui::Text("Granted Gameplay Effects:");
					if (CardAbilitySet->GrantedGameplayEffects.Num() == 0)
					{
						ImGui::Text("%s has no granted gameplay effects.",TCHAR_TO_ANSI(*CardAbilitySet->GetName()));
					}
					for (const FGASCourseCardAbilitySet_GameplayEffect& GameplayEffectEntry : CardAbilitySet->GrantedGameplayEffects)
					{
						TSoftClassPtr<UGameplayEffect> SoftGameplayEffectRef = TSoftClassPtr<UGameplayEffect>(GameplayEffectEntry.GameplayEffect.ToSoftObjectPath());
						if (UGameplayEffect* LoadedGameplayEffect = SoftGameplayEffectRef.LoadSynchronous()->GetDefaultObject<UGameplayEffect>())
						{
							const FString GameplayEffectName = LoadedGameplayEffect->GetName();
							ImGui::Text("%s", TCHAR_TO_ANSI(*GameplayEffectName));
#if WITH_EDITOR
							OpenBlueprintAsset(Cast<UBlueprint>(LoadedGameplayEffect->GetClass()->ClassGeneratedBy));
#endif
						}
					}
				}
				
				ImGui::TableNextColumn();
				if (ImGui::Button("Add to Hand"))
				{
					if (DeckManagerComponent)
					{
						FCardInstance CardInstanceToAdd;
						CardInstanceToAdd.CardInstanceId = FGuid::NewGuid();
						CardInstanceToAdd.CardLevel = CardLevelOverrides.FindOrAdd(LoadedCard->CardData.CardAbilitySet);
						CardInstanceToAdd.CardDataAsset = LoadedCard;
						DeckManagerComponent->ForceCardInstanceInHand(CardInstanceToAdd);
					}
				}
				if (LoadedCard->CardData.CardAbilitySet)
				{
					float CardLevelOverride = GetOrAddCardLevelOverride(LoadedCard->CardData.CardAbilitySet);
					if (ImGui::InputFloat("Card Level Override", &CardLevelOverride, 1, 1))
					{
						CardLevelOverrides.FindOrAdd(LoadedCard->CardData.CardAbilitySet) = CardLevelOverride;
					}
				}
				
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::End();
}

void FGASC_PlayerDeckManagerPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
	CachedPawns = Pawns;
}


TArray<FAssetData> FGASC_PlayerDeckManagerPanel::CacheCardAssets()
{
	TArray<FAssetData> CardAssets;

	for (const TWeakObjectPtr<APawn>& PawnPtr : CachedPawns)
	{
		APawn* Pawn = PawnPtr.Get();
		if (!Pawn || !Pawn->GetWorld())
		{
			continue;
		}

		const AGASCourseCharacter* Character = Cast<AGASCourseCharacter>(Pawn);
		if (!Character)
		{
			continue;
		}

		AGASCoursePlayerState* PlayerState = Pawn->GetPlayerState<AGASCoursePlayerState>();
		if (!PlayerState)
		{
			continue;
		}

		DeckManagerComponent = PlayerState->GetDeckManagerComponent();
		if (!DeckManagerComponent)
		{
			continue;
		}

		DeckManagerComponent->FindAllCards(CardAssets);
	}

	return CardAssets;
}

#if WITH_EDITOR
void FGASC_PlayerDeckManagerPanel::OpenAsset(UObject* AssetObject, bool bForceOpen)
{
	if (GEditor)
	{
		if (bForceOpen)
		{
			TArray<UObject*> ObjectsToSync;
			ObjectsToSync.Add(AssetObject);
			GEditor->SyncBrowserToObjects(ObjectsToSync, true);
									
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				AssetEditorSubsystem->OpenEditorForAsset(AssetObject);
			}
		}
		else
		{
			FString Label = FString::Printf(TEXT("Open##%s:BlueprintObject"), *AssetObject->GetName());
			//ImGui::Text("%s", TCHAR_TO_ANSI(*CardDataDirectoryPath));
			if (ImGui::Button(TCHAR_TO_UTF8(*Label)))
			{
				TArray<UObject*> ObjectsToSync;
				ObjectsToSync.Add(AssetObject);
				GEditor->SyncBrowserToObjects(ObjectsToSync, true);
									
				if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				{
					AssetEditorSubsystem->OpenEditorForAsset(AssetObject);
				}
			}
		}
	}
}



void FGASC_PlayerDeckManagerPanel::OpenBlueprintAsset(UObject* BlueprintObject)
{
	if (GEditor)
	{
		FString Label = FString::Printf(TEXT("Open##%s:BlueprintObject"), *BlueprintObject->GetName());
		if (ImGui::Button(TCHAR_TO_UTF8(*Label)))
		{
			TArray<UObject*> ObjectsToSync;
			ObjectsToSync.Add(BlueprintObject);
			GEditor->SyncBrowserToObjects(ObjectsToSync, true);
									
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				AssetEditorSubsystem->OpenEditorForAsset(BlueprintObject);
			}
		}
	}
}
#endif
