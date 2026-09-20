#pragma once

#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Types.h"
#include "GASC_MeleeSubsystem_Settings.generated.h"

UCLASS(Config=Game, defaultconfig, meta=(DisplayName="GASCourse Melee Trace System Settings"))
class GASCOURSE_API UGASC_MeleeSubsystem_Settings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category="Hits")
	EGASC_MeleeHitPolicy DefaultHitPolicy = EGASC_MeleeHitPolicy::OncePerTarget;
	UPROPERTY(Config, EditAnywhere, Category="Hits", meta=(ClampMin="0"))
	float DefaultRehitDelay = 0.2f;
	UPROPERTY(Config, EditAnywhere, Category="Hits", meta=(ClampMin="0"))
	int32 DefaultRank = 0;
	UPROPERTY(Config, EditAnywhere, Category="Hits", meta=(ClampMin="0"))
	int32 DefaultGroup = 0;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Collision")
	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjectTypes;
	/** Gameplay runs only on authority by default. Client shapes can still be visualized. */
	UPROPERTY(Config, EditAnywhere, Category="Collision")
	bool bAuthorityOnly = true;
	UPROPERTY(Config, EditAnywhere, Category="Collision")
	bool bCheckObstacles = false;
	UPROPERTY(Config, EditAnywhere, Category="Collision")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_Visibility;
	/** Maximum translation/outer-edge arc distance per interpolation step (cm). */
	UPROPERTY(Config, EditAnywhere, Category="Interpolation", meta=(ClampMin="0.1"))
	float MaxInterpolationDistance = 10.0f;
	UPROPERTY(Config, EditAnywhere, Category="Interpolation", meta=(ClampMin="0.1", ClampMax="90"))
	float MaxInterpolationAngle = 5.0f;
	/** Safety budget. Sweeps still span the full interval if exceeded; debug records flag it. */
	UPROPERTY(Config, EditAnywhere, Category="Interpolation", meta=(ClampMin="1", ClampMax="4096"))
	int32 MaxInterpolationSteps = 128;
	/** Fill gaps between authored samples using the shape's smallest extent. */
	UPROPERTY(Config, EditAnywhere, Category="Interpolation")
	bool bAutoTraceDensity = true;
	/** Pad sweeps to cover rotation between sampled poses. May accept near-edge contacts. */
	UPROPERTY(Config, EditAnywhere, Category="Interpolation")
	bool bConservativeRotationCoverage = true;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawDebug = false;
	/** Bounds live and replay geometry independently of collision and history capture. */
	UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(ClampMin="1", ClampMax="4096"))
	int32 MaxDebugDrawSamples = 256;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawShapes = true;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawInterpolatedShapes = true;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawSweepPaths = true;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawHitPoints = true;
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	bool bDrawLabels = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Debug")
	FColor MeleeTraceDebugColor = FColor::Yellow;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Debug")
	FColor MeleeTraceHitDebugColor = FColor::Red;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Debug")
	FColor MeleeTraceInterpolatedDebugColor = FColor::Yellow;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Debug", meta=(ClampMin="0"))
	float DebugDrawTime = 1.0f;
	UPROPERTY(Config, EditAnywhere, Category="History")
	bool bRecordHistory = false;
	UPROPERTY(Config, EditAnywhere, Category="History", meta=(ClampMin="1"))
	float HistoryDuration = 15.0f;
	/** Bounds memory even during long pauses or unusually dense traces. */
	UPROPERTY(Config, EditAnywhere, Category="History", meta=(ClampMin="1"))
	int32 MaxHistorySamples = 30000;
	UPROPERTY(Config, EditAnywhere, Category="History")
	bool bFollowRewindDebugger = false;
	UGASC_MeleeSubsystem_Settings();
};
