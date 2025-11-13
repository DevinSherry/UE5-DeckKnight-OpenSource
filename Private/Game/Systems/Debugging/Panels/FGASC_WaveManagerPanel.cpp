// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Debugging/Panels/FGASC_WaveManagerPanel.h"
#include "imgui.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Game/Systems/WaveManager/GASC_WaveManagerSystemSettings.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

TSubclassOf<APawn> FGASC_WaveManagerPanel::SelectedEnemyToSpawn = nullptr;

FGASC_WaveManagerPanel::FGASC_WaveManagerPanel()
{
	const UGASC_WaveManagerSystemSettings* WaveManagerSystemSettings = GetDefault<UGASC_WaveManagerSystemSettings>();
	EnemyList = WaveManagerSystemSettings->EnemyList;
}

FGASC_WaveManagerPanel::~FGASC_WaveManagerPanel()
{
}

void FGASC_WaveManagerPanel::DrawDebugPanel(bool& bOpen)
{
	if (!bOpen)
	{
		return;
	}

	if (!ImGui::Begin("Wave Manager", &bOpen))
	{
		ImGui::End();
		return;
	}

	if (EnemyListLoaded.Num() == 0)
	{
		RequestAsyncEnemyLoad();
	}
	
	FString ComboLabel = SelectedEnemyToSpawn
			? SelectedEnemyToSpawn->GetName() 
			: FString("None");
	FTCHARToUTF8 ComboLabelConverter(*ComboLabel);

	if (EnemyListLoaded.Num() == 0)
	{
		for (const TSoftClassPtr<APawn>& It : EnemyList)
		{
			EnemyListLoaded.AddUnique(It.LoadSynchronous());
		}
	
	}

	if (ImGui::BeginCombo("EnemyList", ComboLabelConverter.Get()))
	{
		for (const TSubclassOf<APawn>& It : EnemyListLoaded)
		{
			if (!It)
				continue;
			const FString AssetName = It->GetName();
			const FTCHARToUTF8 NameUtf8(*AssetName);
			const char* NameCStr = NameUtf8.Get();

			const bool bIsSelected =
				(SelectedEnemyToSpawn && It->GetFName() == SelectedEnemyToSpawn->GetFName());

			if (ImGui::Selectable(NameCStr, bIsSelected))
			{
				SelectedEnemyToSpawn = It;
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Spawn Wave"))
	{
		UWorld* World = nullptr;
		if (GEngine && GEngine->GetWorldContexts().Num() > 0)
		{
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
			{
				if (WorldContext.World() && WorldContext.WorldType == EWorldType::Game)
				{
					World = WorldContext.World();
					break;
				}
				if (WorldContext.World() && WorldContext.WorldType == EWorldType::PIE)
				{
					World = WorldContext.World();
					break;
				}
			}

			if (World)
			{
				if (SelectedEnemyToSpawn)
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawn Wave: %s"), *SelectedEnemyToSpawn->GetName());
					if (APlayerController* PlayerController = World->GetFirstPlayerController())
					{
						APawn* EnemyToSpawn = nullptr;
						if (APawn* PlayerCharacter = PlayerController->GetPawn())
						{
							FVector PawnLocation = PlayerCharacter->GetActorLocation();
							FVector PawnForward = PlayerCharacter->GetActorForwardVector();
							FVector ConeVector = FMath::VRandCone(PawnForward, 0.8f);
							FNavLocation NavLoc;
							UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
							if (NavSys && NavSys->ProjectPointToNavigation(PawnLocation + (ConeVector * 600.0f), NavLoc, FVector(500.0f, 500.0f, 500.0f)))
							{
								FTransform SpawnTransform = FTransform(FRotator(0.0f), NavLoc.Location, FVector(1.0f));
								DrawDebugSphere(World, NavLoc.Location, 50.0f, 10, FColor::Red, false, 5.0f, 0, 2.0f);
								EnemyToSpawn = World->SpawnActorDeferred<APawn>(SelectedEnemyToSpawn, SpawnTransform, PlayerCharacter, PlayerCharacter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
								EnemyToSpawn->FinishSpawning(SpawnTransform);
							}
						}
					}
				}
			}
		}
	}

	ImGui::End();
}

void FGASC_WaveManagerPanel::UpdateCachedPawns(TArray<TWeakObjectPtr<APawn>> Pawns)
{
}

void FGASC_WaveManagerPanel::RequestAsyncEnemyLoad()
{
	// Lazy load the enemy list if it's empty
	if (EnemyList.Num() == 0)
	{
		if (const UGASC_WaveManagerSystemSettings* WaveManagerSystemSettings = GetDefault<UGASC_WaveManagerSystemSettings>())
		{
			EnemyList = WaveManagerSystemSettings->EnemyList;
		}
	}

	if (EnemyList.Num() == 0)
	{
		return;
	}

	TArray<FSoftObjectPath> SoftObjectPaths;
	for (const TSoftClassPtr<APawn>& It : EnemyList)
	{
		if (It.IsValid())
		{
			EnemyListLoaded.AddUnique(It.Get());
		}
		else
		{
			SoftObjectPaths.Add(It.ToSoftObjectPath());
		}
	}

	if (SoftObjectPaths.Num() == 0)
	{
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	TSharedPtr<FGASC_WaveManagerPanel> PanelSharedPtr = MakeShared<FGASC_WaveManagerPanel>();
	TWeakPtr<FGASC_WaveManagerPanel> WeakThis = PanelSharedPtr;
	StreamableManager.RequestAsyncLoad(SoftObjectPaths, [WeakThis, SoftObjectPaths]()
	{
		if (auto This = WeakThis.Pin())
		{
			for (const FSoftObjectPath& Path : SoftObjectPaths)
			{
				if (UClass* LoadedClass = Cast<UClass>(Path.ResolveObject()))
				{
					This->EnemyListLoaded.AddUnique(LoadedClass);
				}
				else if (UClass* Loaded = Cast<UClass>(Path.TryLoad()))
				{
					This->EnemyListLoaded.AddUnique(Loaded);
				}
			}
		}
	},
	FStreamableManager::AsyncLoadHighPriority);
}
