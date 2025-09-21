// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Panels/FGASCDebugHub.h"
#include "GASC_DebugSubsystem.generated.h"

/**
 * @class UGASC_DebugSubsystem
 * @brief A subsystem designed for debugging purposes within the application.
 *
 * This class is responsible for managing and facilitating debugging
 * features, including logging, monitoring, and diagnostic tools,
 * that help developers identify and solve issues in the system.
 *
 * The UGASC_DebugSubsystem integrates seamlessly with other
 * subsystems and provides essential debugging functionalities to monitor
 * and analyze system behavior.
 *
 * It is designed with flexibility and extensibility in mind, allowing
 * customization or enhancements, as needed, during system development
 * and maintenance.
 */
UCLASS()
class GASCOURSE_API UGASC_DebugSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// FTickableGameObject implementation Begin
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// FTickableGameObject implementation End

	FGASCDebugHub& GetDebugHub() { return DebugHub; }

	UFUNCTION(BlueprintCallable, Category = "GAS Course|Debug")
	void ToggleGameplayDebugHUD(bool bInOpen);

	UFUNCTION(BlueprintCallable, Category = "GAS Course|Debug")
	bool IsDebugOpen() const;

private:
	void DrawImGui();

	void CacheAllPawns(UWorld* World);
	
	TArray<TWeakObjectPtr<APawn>> CachedPawns;
	FGASCDebugHub DebugHub;
};
