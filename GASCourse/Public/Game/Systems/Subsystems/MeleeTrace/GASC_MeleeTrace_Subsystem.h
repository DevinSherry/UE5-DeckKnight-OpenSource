// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "GASCourse/Public/Game/Systems/Subsystems/MeleeTrace/Shapes/GASC_MeleeShape_Base.h"
#include "Misc/Guid.h"
#include "CollisionShape.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Game/Systems/Subsystems/MeleeTrace/Settings/GASC_MeleeSubsystem_Settings.h" 
#include "GASC_MeleeTrace_Subsystem.generated.h"

struct FGASC_MeleeActorFrame;
class FGASC_RewindGhostRenderer;

DECLARE_LOG_CATEGORY_EXTERN(LOG_GASC_MeleeTraceSubsystem, Log, All);

UENUM(BlueprintType)
enum class EGASC_MeleeTrace_TraceShape : uint8
{
	Box,
	Capsule,
	Sphere 
};

UENUM(BlueprintType)
enum class EGASC_MeleeTrace_TraceObject : uint8
{
	CharacterMesh,
	Weapon,
};

USTRUCT(BlueprintType)
struct FGASC_MeleeTrace_TraceShapeData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Trace|Shape Data")
	EGASC_MeleeTrace_TraceShape TraceShape = EGASC_MeleeTrace_TraceShape::Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Shape Data", meta=(EditCondition = "TraceShape == EGASC_MeleeTrace_TraceShape::Sphere"))
	float SphereRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Shape Data", meta=(EditCondition = "TraceShape == EGASC_MeleeTrace_TraceShape::Capsule"))
	float CapsuleRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Shape Data", meta=(EditCondition = "TraceShape == EGASC_MeleeTrace_TraceShape::Capsule"))
	float CapsuleHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Shape Data", meta=(EditCondition = "TraceShape == EGASC_MeleeTrace_TraceShape::Box"))
	FVector BoxExtent = FVector::Zero();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Socket Data")
	EGASC_MeleeTrace_TraceObject TraceObject = EGASC_MeleeTrace_TraceObject::Weapon;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Socket Data")
	FName StartSocket = FName("WeaponTrace_Start");
	/** Character socket to find the attached weapon under. None preserves automatic weapon lookup.
	 * Requires a static/skeletal mesh with the trace sockets; use MeshComponentNameOrTag to disambiguate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(EditCondition="TraceObject == EGASC_MeleeTrace_TraceObject::Weapon", DisplayName="Weapon Attachment Socket"))
	FName WeaponAttachmentSocket = NAME_None;
	/** Instance to trace on an InstancedStaticMeshComponent. Ignored for ordinary mesh components. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(ClampMin="0"))
	int32 MeshInstanceIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	EGASC_MeleeSocketMode SocketMode = EGASC_MeleeSocketMode::StartAndEnd;
	/** Align this part of the authored shape to each socket/segment sample, after RotationOffset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	EGASC_MeleeShapeAnchor ShapeAnchor = EGASC_MeleeShapeAnchor::Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Socket Data", meta=(EditCondition="SocketMode == EGASC_MeleeSocketMode::StartAndEnd"))
	FName EndSocket = FName("WeaponTrace_End");
	/** Single-socket mode: aim local +Z from StartSocket toward this bone/socket on the same mesh.
	 * None uses the start socket rotation. Does not add samples or change shape length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(EditCondition="SocketMode == EGASC_MeleeSocketMode::SingleSocket"))
	FName OrientationSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Socket Data")
	int32 TraceDensity = 2;

	/** Stable designer label, included in hit events and debug history. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	FName ShapeName = NAME_None;
	/** Lower ranks win among contacts with the same target/group in an update. -1 uses settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits", meta=(ClampMin="-1"))
	int32 Rank = -1;
	/** Shapes in a group share target hit history. Different groups may intentionally hit separately. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits", meta=(ClampMin="-1"))
	int32 Group = -1;
	/** Passed into the existing damage pipeline to author sweetspot-specific responses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	FGameplayTagContainer HitContextTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FVector LocalOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FRotator RotationOffset = FRotator::ZeroRotator;
	/** Align shape Z with the socket segment. Otherwise use the start socket orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	bool bAlignToSocketSegment = true;
	/** Optional component name/tag to select off-hand weapons or additional character meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FName MeshComponentNameOrTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	bool bIgnoreObstacles = false;
	
};

/**
 * FGASC_MeleeTrace_Subsystem_Data is a data structure used for configuring and handling melee trace functionality.
 * It encapsulates configuration details required for performing melee traces, including trace shape, trace sockets,
 * trace density, and runtime metadata such as source mesh component, instigator actor, and collision data.
 *
 * This structure serves as the core data exchanged and processed by the melee trace subsystem, enabling precise
 * collision detection and multi-frame sample tracking during melee interactions.
 */
USTRUCT(BlueprintType)
struct FGASC_MeleeTrace_Subsystem_Data
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Instanced, Category = "Melee Trace")
	TObjectPtr<UGASC_MeleeShape_Base> TraceShape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace")
	FName TraceSocket_Start = NAME_None;
	/** Optional character attachment socket restricting weapon selection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(EditCondition="TraceObject == EGASC_MeleeTrace_TraceObject::Weapon"))
	FName WeaponAttachmentSocket = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(ClampMin="0"))
	int32 MeshInstanceIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	EGASC_MeleeSocketMode SocketMode = EGASC_MeleeSocketMode::StartAndEnd;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	EGASC_MeleeShapeAnchor ShapeAnchor = EGASC_MeleeShapeAnchor::Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace")
	FName TraceSocket_End = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data", meta=(EditCondition="SocketMode == EGASC_MeleeSocketMode::SingleSocket"))
	FName OrientationSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace|Socket Data")
	EGASC_MeleeTrace_TraceObject TraceObject = EGASC_MeleeTrace_TraceObject::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Trace")
	int32 TraceDensity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	FName ShapeName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	int32 Rank = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	int32 Group = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	FGameplayTagContainer HitContextTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FVector LocalOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FRotator RotationOffset = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	bool bAlignToSocketSegment = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Socket Data")
	FName MeshComponentNameOrTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	bool bIgnoreObstacles = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace|Hits")
	EGASC_MeleeHitPolicy HitPolicy = EGASC_MeleeHitPolicy::UseProjectDefault;
	FTransform PreviousSocketTransform = FTransform::Identity;
	FVector PreviousEndLocal = FVector::ZeroVector;
	FVector PreviousOrientationLocal = FVector::ZeroVector;
	bool UsesOrientationSocket() const { return SocketMode == EGASC_MeleeSocketMode::SingleSocket && !OrientationSocket.IsNone(); }
	int32 ShapeIndex = 0;

	TWeakObjectPtr<UMeshComponent> SourceMeshComponent = nullptr;
	
	UPROPERTY()
	AActor* InstigatorActor = nullptr;
	
	FCollisionShape TraceCollisionShape;
	TArray<FVector> PreviousFrameSamples;
	
	UPROPERTY()
	TArray<AActor*> HitActors;
	
	UPROPERTY()
	TArray<AActor*> HitActors_PreviousFrames;
	
	UPROPERTY()
	TArray<FHitResult> HitResults_PreviousFrames;
	
	UPROPERTY()
	TMap<TWeakObjectPtr<AActor>, float> PerActorHitStamps;
	
	float SwingStartTime = 0.0f;
	float HitCooldownTime = -1.0f;
	
	FGuid TraceId;

	FGASC_MeleeTrace_Subsystem_Data() {}
};

/** One recorded sweep. Value data survives actor destruction; no UObject ownership. */
struct FGASC_MeleeDebugSample
{
	double Time = 0;
	double FrameStartTime = 0;
	double RecordingTime = -1;
	double RecordingFrameStartTime = -1;
	uint64 Frame = 0;
	TWeakObjectPtr<AActor> Actor;
	FString ActorName;
	FName ShapeName;
	FGuid WindowId;
	int32 ShapeIndex = 0;
	int32 Rank = 0;
	int32 Group = 0;
	EGASC_MeleeHitPolicy Policy = EGASC_MeleeHitPolicy::OncePerTarget;
	float RehitDelay = 0;
	FCollisionShape Shape;
	FTransform Start;
	FTransform End;
	bool bInterpolated = false;
	bool bHit = false;
	bool bAccepted = false;
	bool bBudgetLimited = false;
	TArray<FVector> ImpactPoints;
};

struct FGASC_MeleeDebugOptions
{
	bool bEnabled = false;
	bool bShapes = true;
	bool bInterpolated = true;
	bool bPaths = true;
	bool bHitPoints = true;
	bool bLabels = true;
	bool bRecord = false;
	bool bFollowRewind = false;
	bool bPlayback = false;
	bool bCaptureActors = true, bActorGhosts = true, bActorBounds = true;
	int32 MaxGhostActors = 16;
	float ActorCaptureBudgetMs = 1.f;
	float DrawDuration = 1;
	double PlaybackTime = 0;
	TSet<TWeakObjectPtr<AActor>> HiddenActors;
};

struct FGASC_MeleeTraceWindow
{
	// Editor preview must validate the playhead, including seeks that do not fire NotifyEnd.
	bool bNotifyPreview = false;
	TWeakObjectPtr<USkeletalMeshComponent> PreviewMesh;
	TWeakObjectPtr<UMeshComponent> PreviewWeapon;
	TWeakObjectPtr<const UObject> PreviewAnimation;
	float PreviewStart = 0.f;
	float PreviewEnd = 0.f;
	TWeakObjectPtr<AActor> Owner;
	EGASC_MeleeHitPolicy Policy = EGASC_MeleeHitPolicy::OncePerTarget;
	float RehitDelay = 0;
	bool bClosing = false;
	FGASC_MeleeHitLedger Ledger;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FGASC_MeleeTraceHit, FGuid, WindowId, FName, ShapeName, int32, Rank, int32, Group, FHitResult, Hit, FGameplayTagContainer, HitContextTags);

/**
 * UGASC_MeleeTrace_Subsystem is a tickable world subsystem responsible for handling and managing melee trace requests.
 * This subsystem processes queued melee trace data and executes the necessary functionality each tick cycle.
 */
UCLASS()
class GASCOURSE_API UGASC_MeleeTrace_Subsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	/** Accepted hits after rank/group and policy resolution. */
	UPROPERTY(BlueprintAssignable, Category="GASCourse|MeleeTrace")
	FGASC_MeleeTraceHit OnMeleeTraceHit;

	/** Add all shapes atomically to one independently owned notify window. */
	UFUNCTION(BlueprintCallable, Category="GASCourse|MeleeTrace")
	void RequestMeleeTraceWindow(AActor* Instigator, const TArray<FGASC_MeleeTrace_Subsystem_Data>& Shapes,
		FGuid WindowId, EGASC_MeleeHitPolicy HitPolicy = EGASC_MeleeHitPolicy::UseProjectDefault, float RehitDelay = -1.0f);

	FGASC_MeleeDebugOptions DebugOptions;
	uint64 DroppedDebugSamples = 0;
	const TArray<FGASC_MeleeDebugSample>& GetDebugHistory() const { return DebugHistory; }
	const TArray<FGASC_MeleeTrace_Subsystem_Data>& GetActiveTraces() const { return MeleeTraceRequests; }
	void ClearDebugHistory();
	void ClearDebugGhosts();
	void PruneDebugActors();
	void CaptureDebugActors(const TArray<FGASC_MeleeDebugSample>& Samples, const TArray<AActor*>& Actors);
	uint64 DroppedActorFrames = 0;
	SIZE_T ActorHistoryBytes = 0;
	double LastActorCaptureMs = 0;
	void SetNotifyPreviewContext(FGuid WindowId, USkeletalMeshComponent* Mesh, const UObject* Animation, float Start, float End);
	void SetNotifyPreviewWeapon(FGuid WindowId, UMeshComponent* Weapon);
	UMeshComponent* GetNotifyPreviewWeapon(FGuid WindowId) const;
	void DrawDebugHistory(UWorld* World, double Time, bool bRecordingTime = false) const;
	void DrawDebugSample(UWorld* World, const FGASC_MeleeDebugSample& Sample, float Duration, bool bDrawLabel = true) const;
	void DrawDebugSamples(UWorld* World, const TArray<FGASC_MeleeDebugSample>& Samples, uint64 Frame = MAX_uint64) const;
	void UpdateLiveDebugSamples(const TArray<FGASC_MeleeDebugSample>& Samples);

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void DrawDebugMeleeTrace(const UObject* WorldContextObject, const FCollisionShape& MeleeTraceShape, const FTransform& Start, const FTransform& End,
		bool bHit, const TArray<FHitResult>& HitResults);

	void DrawDebugSphereTraceMulti(const UWorld* World,
		const FVector& Start,
		const FVector& End,
		float Radius,
		EDrawDebugTrace::Type DrawDebugType,
		bool bHit,
		const TArray<FHitResult>& Hits,
		const FLinearColor& TraceColor,
		const FLinearColor& TraceHitColor,
		float DrawTime);
	
	void DrawDebugSweptSphere(const UWorld* InWorld,
		FVector const& Start,
		FVector const& End,
		float Radius,
		FColor const& Color,
		bool bPersistentLines,
		float LifeTime,
		uint8 DepthPriority = 0);

	void DrawDebugCapsuleTraceMulti(const UWorld* World,
	const FVector& Start,
	const FVector& End,
	const FQuat& Orientation,
	float Radius,
	float HalfHeight,
	EDrawDebugTrace::Type DrawDebugType,
	bool bHit,
	const TArray<FHitResult>& HitResults,
	const FLinearColor& TraceColor,
	const FLinearColor& TraceHitColor,
	float DrawTime);

	void DrawDebugBoxTraceMulti(const UWorld* World,
	const FVector& Start,
	const FVector& End,
	const FRotator& Orientation,
	const FVector& BoxExtent,
	EDrawDebugTrace::Type DrawDebugType,
	bool bHit,
	const TArray<FHitResult>& HitResults,
	const FLinearColor& TraceColor,
	const FLinearColor& TraceHitColor,
	float DrawTime);

	void DrawDebugSweptBox(const UWorld* InWorld,
	FVector const& Start,
	FVector const& End,
	FRotator const& Orientation,
	FVector const& HalfSize,
	FColor const& Color,
	bool bPersistentLines,
	float LifeTime,
	uint8 DepthPriority = 0);

	UFUNCTION(BlueprintCallable, Category="GASCourse|MeleeTrace")
	void RequestShapeMeleeTrace(AActor* Instigator, FGASC_MeleeTrace_Subsystem_Data TraceData, FGuid TraceId);

	UFUNCTION(BlueprintCallable, Category="GASCourse|MeleeTrace")
	bool IsMeleeTraceInProgress(FGuid TraceId);

	FGASC_MeleeTrace_Subsystem_Data CreateShapeDataFromRow(const FGASC_MeleeTrace_TraceShapeData& RowData) const;

	UFUNCTION(BlueprintCallable, Category="GASCourse|MeleeTrace")
	bool CancelMeleeTrace(FGuid TraceId);

	UFUNCTION()
	TWeakObjectPtr<UMeshComponent> GetMeshComponent(const AActor* Actor, const FGASC_MeleeTrace_Subsystem_Data& InTraceData);

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGASC_MeleeTrace_Subsystem, STATGROUP_Tickables);
	}

	static void GetTraceSamples(const UMeshComponent* MeshComponent,
	int32 TraceDensity,
	const FName& StartSocketName,
	const FName& EndSocketName,
	TArray<FVector>& OutSamples, int32 MeshInstanceIndex = 0);

private:
	friend class FGASC_MeleeTraceWorldTest;
	friend class FGASC_MeleeWeaponAttachmentTest;
	friend class FGASC_MeleeGhostHistoryTest;
#if !UE_BUILD_SHIPPING
	TArray<TSharedPtr<FGASC_MeleeActorFrame>> ActorFrames;
	mutable TSharedPtr<FGASC_RewindGhostRenderer> ActorGhostRenderer;
#endif
	
	UPROPERTY(Transient)
	TArray<FGASC_MeleeTrace_Subsystem_Data> MeleeTraceRequests;
	TMap<FGuid, FGASC_MeleeTraceWindow> Windows;
	TArray<FGASC_MeleeDebugSample> DebugHistory;
	TArray<FGASC_MeleeDebugSample> LiveDebugSamples;
	FDelegateHandle PostActorTickHandle;
	double LastRecordingTime = 0;
	bool bProcessing = false;
	TSet<FGuid> CanceledDuringDispatch;
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaTime);
	void PrunePreviewWindows();
	void SynchronizePreviewAttachments();
	void DispatchHit(const FGASC_MeleeTrace_Subsystem_Data& Trace, const FHitResult& Hit);

	void ProcessMeleeTraces(float DeltaTime);

	FCollisionObjectQueryParams ConfigureCollisionObjectParams(const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes);
	
	UPROPERTY()
	const UGASC_MeleeSubsystem_Settings* MeleeTraceSettings = nullptr;
};
