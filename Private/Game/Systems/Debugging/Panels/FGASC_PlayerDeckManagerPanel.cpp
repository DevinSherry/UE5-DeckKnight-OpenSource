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
	
	if (ImGui::BeginTable("All Cards in Game Table", 4, 
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
		
		for (int32 i = 0; i < CachedCardDataAssets.Num(); ++i)
		{
			const FAssetData& AssetData = CachedCardDataAssets[i];
			if (!AssetData.IsValid()) continue;
			
			SelectedCardAsset = TSoftObjectPtr<UCardDataAsset>(AssetData.ToSoftObjectPath());
			if (UCardDataAsset* LoadedCard = SelectedCardAsset.LoadSynchronous())
			{
				ImGui::PushID(i); // ✅ stable unique ID
				ImGui::TableNextRow();
			
				ImGui::TableNextColumn();
				
				const FString CardDataAssetName = AssetData.AssetName.ToString();
				const FString CardDataDirectoryPath = AssetData.PackagePath.ToString();
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardDataAssetName));
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardDataDirectoryPath));
			
				ImGui::TableNextColumn();
				
				const FString CardName = LoadedCard->CardData.CardInfo.CardName.ToString().IsEmpty() ? "Empty" :
				LoadedCard->CardData.CardInfo.CardName.ToString();
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardName));
				
				const FString CardDescription = LoadedCard->CardData.CardInfo.CardDescription.ToString().IsEmpty() ? "Empty" :
				LoadedCard->CardData.CardInfo.CardDescription.ToString();
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardDescription));
				
				const FString CardSlotType =
				StaticEnum<ECardType>()->GetNameStringByValue((int64)LoadedCard->CardData.CardInfo.CardType);
				ImGui::Text("%s", TCHAR_TO_ANSI(*CardSlotType));
				
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
				}
				
				ImGui::TableNextColumn();
				
				if (UBaseCardGameplayAbilitySet* CardAbilitySet = LoadedCard->CardData.CardAbilitySet)
				{
					ImGui::Text("Granted Gameplay Abilities:");
					if (CardAbilitySet->GrantedGameplayAbilities.Num() == 0)
					{
						ImGui::Text("%s has no granted gameplay abilities.",TCHAR_TO_ANSI(*CardName));
					}
					for (const FGASCourseCardAbilitySet_GameplayAbility& AbilityEntry : CardAbilitySet->GrantedGameplayAbilities)
					{
						TSoftClassPtr<UGASCourseGameplayAbility> SoftAbilityRef = TSoftClassPtr<UGASCourseGameplayAbility>(AbilityEntry.GameplayAbility.ToSoftObjectPath());
						if (UGASCourseGameplayAbility* LoadedAbility = SoftAbilityRef.LoadSynchronous()->GetDefaultObject<UGASCourseGameplayAbility>())
						{
							const FString AbilityName = LoadedAbility->GetName();
							ImGui::Text("%s", TCHAR_TO_ANSI(*AbilityName));
							
							const FString AbilityType =
							StaticEnum<EGASCourseAbilitySlotType>()->GetNameStringByValue((int64)AbilityEntry.AbilitySlotType);
							ImGui::Text("%s", TCHAR_TO_ANSI(*AbilityType));
						}
					}
					ImGui::Text("Granted Gameplay Effects:");
					if (CardAbilitySet->GrantedGameplayEffects.Num() == 0)
					{
						ImGui::Text("%s has no granted gameplay effects.",TCHAR_TO_ANSI(*CardName));
					}
					for (const FGASCourseCardAbilitySet_GameplayEffect& GameplayEffectEntry : CardAbilitySet->GrantedGameplayEffects)
					{
						TSoftClassPtr<UGameplayEffect> SoftGameplayEffectRef = TSoftClassPtr<UGameplayEffect>(GameplayEffectEntry.GameplayEffect.ToSoftObjectPath());
						if (UGameplayEffect* LoadedGameplayEffect = SoftGameplayEffectRef.LoadSynchronous()->GetDefaultObject<UGameplayEffect>())
						{
							const FString GameplayEffectName = LoadedGameplayEffect->GetName();
							ImGui::Text("%s", TCHAR_TO_ANSI(*GameplayEffectName));
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

