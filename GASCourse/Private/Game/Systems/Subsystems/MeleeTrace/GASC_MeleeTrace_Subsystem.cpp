// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Subsystem.h"
#include "KismetTraceUtils.h"
#include "Game/Systems/Subsystems/MeleeTrace/Shapes/GASC_MeleeShape_Base.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Game/GameplayAbilitySystem/GASCourseAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimMontage.h"
#include "EngineUtils.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_NotifyState.h"
#include "ObjectTrace.h"
#include "GASCourse/GASCourseCharacter.h"
#include "NativeGameplayTags.h"
#include "Game/GameplayAbilitySystem/GASCourseNativeGameplayTags.h"
#include "Game/Character/Player/GASCoursePlayerCharacter.h"

static const float KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE = 8.f;
DEFINE_LOG_CATEGORY(LOG_GASC_MeleeTraceSubsystem);

namespace
{
	bool HasTraceInstance(const UMeshComponent* Mesh, int32 InstanceIndex)
	{
		const auto* Instances = Cast<UInstancedStaticMeshComponent>(Mesh);
		return !Instances || (InstanceIndex >= 0 && InstanceIndex < Instances->GetInstanceCount());
	}

	FTransform GetTraceSocketTransform(const UMeshComponent* Mesh, FName Socket, int32 InstanceIndex)
	{
		if (const auto* Instances = Cast<UInstancedStaticMeshComponent>(Mesh))
		{
			FTransform InstanceWorld;
			if (Instances->GetInstanceTransform(InstanceIndex, InstanceWorld, true))
				return Mesh->GetSocketTransform(Socket, RTS_Component) * InstanceWorld;
		}
		return Mesh->GetSocketTransform(Socket);
	}
}


#if !UE_BUILD_SHIPPING
namespace GASCourse_MeleeSubSystemCVars
{
	static TAutoConsoleVariable<bool> CvarEnableMeleeTracesDebug(TEXT("GASCourseDebug.MeleeTrace.CollisionShapes"),
		false,
		TEXT("Enable on-screen debug collision shape preview for melee traces.(Enabled: true, Disabled: false)"));
	
}
#endif


void UGASC_MeleeTrace_Subsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
	if (DebugOptions.bEnabled && DebugOptions.bPlayback && !DebugOptions.bFollowRewind)
		DrawDebugHistory(GetWorld(), DebugOptions.PlaybackTime);
	else if (!DebugOptions.bFollowRewind) ClearDebugGhosts();
#endif
}

void UGASC_MeleeTrace_Subsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MeleeTraceSettings = GetDefault<UGASC_MeleeSubsystem_Settings>();
	DebugOptions.bEnabled = MeleeTraceSettings->bDrawDebug;
	DebugOptions.bShapes = MeleeTraceSettings->bDrawShapes;
	DebugOptions.bInterpolated = MeleeTraceSettings->bDrawInterpolatedShapes;
	DebugOptions.bPaths = MeleeTraceSettings->bDrawSweepPaths;
	DebugOptions.bHitPoints = MeleeTraceSettings->bDrawHitPoints;
	DebugOptions.bLabels = MeleeTraceSettings->bDrawLabels;
	DebugOptions.bRecord = MeleeTraceSettings->bRecordHistory;
	DebugOptions.bFollowRewind = MeleeTraceSettings->bFollowRewindDebugger;
	DebugOptions.DrawDuration = MeleeTraceSettings->DebugDrawTime;
	PostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &ThisClass::OnWorldPostActorTick);
}

void UGASC_MeleeTrace_Subsystem::Deinitialize()
{
	FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickHandle);
	MeleeTraceRequests.Reset();
	for (auto& Pair : Windows)
		if (auto* Weapon = Pair.Value.PreviewWeapon.Get()) Weapon->DestroyComponent();
	Windows.Reset();
	ClearDebugHistory();
	LiveDebugSamples.Reset();
	Super::Deinitialize();
}

bool UGASC_MeleeTrace_Subsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->WorldType == EWorldType::EditorPreview);
}

void UGASC_MeleeTrace_Subsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void UGASC_MeleeTrace_Subsystem::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	// Skeletal mesh animation and attached weapon transforms have finished updating here.
	if (World != GetWorld()) return;
	const bool bPreview = World->WorldType == EWorldType::EditorPreview;
	if (!bPreview && (TickType == LEVELTICK_ViewportsOnly || World->IsPaused())) return;
	if (bPreview)
	{
		PrunePreviewWindows();
		SynchronizePreviewAttachments();
	}
	if (!MeleeTraceRequests.IsEmpty()) ProcessMeleeTraces(DeltaTime);
#if OBJECT_TRACE_ENABLED
	LastRecordingTime = FObjectTrace::GetWorldElapsedTime(World);
#endif
#if !UE_BUILD_SHIPPING
	const double OldestTime = World->GetTimeSeconds() - FMath::Max(1.f, MeleeTraceSettings->HistoryDuration);
	int32 Expired = 0;
	while (Expired < DebugHistory.Num() && DebugHistory[Expired].Time < OldestTime) ++Expired;
	if (Expired) DebugHistory.RemoveAt(0, Expired, EAllowShrinking::No);
	PruneDebugActors();
	const bool bLiveDraw = (DebugOptions.bEnabled || GASCourse_MeleeSubSystemCVars::CvarEnableMeleeTracesDebug.GetValueOnGameThread()) && !DebugOptions.bPlayback;
	if (!bLiveDraw) LiveDebugSamples.Reset();
	else
	{
		const double DrawCutoff = World->GetTimeSeconds() - FMath::Max(0.f, DebugOptions.DrawDuration);
		LiveDebugSamples.RemoveAll([DrawCutoff, this](const auto& Sample)
		{
			if (GetWorld()->WorldType == EWorldType::EditorPreview)
				return Sample.Frame != GFrameCounter || !Windows.Contains(Sample.WindowId);
			return Sample.Time < DrawCutoff || (DebugOptions.DrawDuration <= 0.f && Sample.Frame != GFrameCounter);
		});
		DrawDebugSamples(World, LiveDebugSamples);
	}
#endif
}

void UGASC_MeleeTrace_Subsystem::DrawDebugMeleeTrace(const UObject* WorldContextObject,
	const FCollisionShape& MeleeTraceShape, const FTransform& Start, const FTransform& End, bool bHit,
	const TArray<FHitResult>& HitResults)
{
#if !UE_BUILD_SHIPPING
	FGASC_MeleeDebugSample Sample;
	Sample.Shape = MeleeTraceShape;
	Sample.Start = Start;
	Sample.End = End;
	Sample.bHit = bHit || !HitResults.IsEmpty();
	for (const FHitResult& Hit : HitResults) Sample.ImpactPoints.Add(Hit.ImpactPoint);
	DrawDebugSample(GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull),
		Sample, DebugOptions.DrawDuration);
#endif
}


void UGASC_MeleeTrace_Subsystem::DrawDebugSphereTraceMulti(const UWorld* World, const FVector& Start,
	const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& Hits,
	const FLinearColor& TraceColor, const FLinearColor& TraceHitColor, float DrawTime)
{
	if (!World || DrawDebugType == EDrawDebugTrace::None) return;
	const bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	const float Lifetime = DrawDebugType == EDrawDebugTrace::ForDuration ? FMath::Max(0.f, DrawTime) : 0.f;
	const FColor Color = (bHit || !Hits.IsEmpty() ? TraceHitColor : TraceColor).ToFColor(true);
	DrawDebugSweptSphere(World, Start, End, Radius, Color, bPersistent, Lifetime);
	for (const auto& Hit : Hits)
		::DrawDebugPoint(World, Hit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceHitColor.ToFColor(true), bPersistent, Lifetime);
}

void UGASC_MeleeTrace_Subsystem::DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End,
	float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;
	float const HalfHeight = (Dist * 0.5f) + Radius;

	FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	::DrawDebugCapsule(InWorld,
		Center,
		HalfHeight,
		Radius,
		CapsuleRot,
		Color,
		bPersistentLines,
		LifeTime,
		DepthPriority);
}


void UGASC_MeleeTrace_Subsystem::DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start,
	const FVector& End, const FQuat& Orientation, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType,
	bool bHit, const TArray<FHitResult>& Hits, const FLinearColor& TraceColor, const FLinearColor& TraceHitColor, float DrawTime)
{
	if (!World || DrawDebugType == EDrawDebugTrace::None) return;
	const bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	const float Lifetime = DrawDebugType == EDrawDebugTrace::ForDuration ? FMath::Max(0.f, DrawTime) : 0.f;
	const FColor Color = (bHit || !Hits.IsEmpty() ? TraceHitColor : TraceColor).ToFColor(true);
	::DrawDebugCapsule(World, Start, HalfHeight, Radius, Orientation, Color, bPersistent, Lifetime);
	::DrawDebugCapsule(World, End, HalfHeight, Radius, Orientation, Color, bPersistent, Lifetime);
	::DrawDebugLine(World, Start, End, Color, bPersistent, Lifetime);
	for (const auto& Hit : Hits)
		::DrawDebugPoint(World, Hit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceHitColor.ToFColor(true), bPersistent, Lifetime);
}

void UGASC_MeleeTrace_Subsystem::DrawDebugBoxTraceMulti(const UWorld* World, const FVector& Start,
	const FVector& End, const FRotator& Orientation, const FVector& BoxExtent, EDrawDebugTrace::Type DrawDebugType,
	bool bHit, const TArray<FHitResult>& Hits, const FLinearColor& TraceColor, const FLinearColor& TraceHitColor, float DrawTime)
{
	if (!World || DrawDebugType == EDrawDebugTrace::None) return;
	const bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	const float Lifetime = DrawDebugType == EDrawDebugTrace::ForDuration ? FMath::Max(0.f, DrawTime) : 0.f;
	const FColor Color = (bHit || !Hits.IsEmpty() ? TraceHitColor : TraceColor).ToFColor(true);
	DrawDebugSweptBox(World, Start, End, Orientation, BoxExtent, Color, bPersistent, Lifetime);
	for (const auto& Hit : Hits)
		::DrawDebugPoint(World, Hit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceHitColor.ToFColor(true), bPersistent, Lifetime);
}

void UGASC_MeleeTrace_Subsystem::DrawDebugSweptBox(const UWorld* InWorld, FVector const& Start, FVector const& End,
	FRotator const& Orientation, FVector const& HalfSize, FColor const& Color, bool bPersistentLines, float LifeTime,
	uint8 DepthPriority)
{
	FVector const TraceVec = End - Start;
	FQuat const CapsuleRot = Orientation.Quaternion();
	::DrawDebugBox(InWorld, Start, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);

	//now draw lines from vertices
	FVector Vertices[8];
	Vertices[0] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z)); //flt
	Vertices[1] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));  //frt
	Vertices[2] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));  //flb
	Vertices[3] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));   //frb
	Vertices[4] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));  //blt
	Vertices[5] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));   //brt
	Vertices[6] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));   //blb
	Vertices[7] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));    //brb
	for (int32 VertexIdx = 0; VertexIdx < 8; ++VertexIdx)
	{
		::DrawDebugLine(InWorld,
			Vertices[VertexIdx],
			Vertices[VertexIdx] + TraceVec,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority);
	}

	::DrawDebugBox(InWorld, End, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
}


void UGASC_MeleeTrace_Subsystem::RequestMeleeTraceWindow(AActor* Instigator,
	const TArray<FGASC_MeleeTrace_Subsystem_Data>& Shapes, FGuid WindowId, EGASC_MeleeHitPolicy HitPolicy, float RehitDelay)
{
	if (!IsValid(Instigator) || !WindowId.IsValid() || Windows.Contains(WindowId)) return;
	for (FGASC_MeleeTrace_Subsystem_Data Shape : Shapes)
	{
		Shape.HitPolicy = HitPolicy;
		Shape.HitCooldownTime = RehitDelay;
		RequestShapeMeleeTrace(Instigator, Shape, WindowId);
	}
}

void UGASC_MeleeTrace_Subsystem::RequestShapeMeleeTrace(AActor* Instigator,
	FGASC_MeleeTrace_Subsystem_Data TraceData, FGuid TraceId)
{
	if (!IsValid(Instigator) || !IsValid(TraceData.TraceShape) || !TraceId.IsValid() || !GetWorld()) return;
	if (const auto* Existing = Windows.Find(TraceId))
		if (Existing->Owner != Instigator || Existing->bClosing) return;

	TraceData.TraceSocket_Start = TraceData.TraceSocket_Start.IsNone() ? FName("Root") : TraceData.TraceSocket_Start;
	TraceData.TraceSocket_End = TraceData.SocketMode == EGASC_MeleeSocketMode::SingleSocket || TraceData.TraceSocket_End.IsNone()
		? TraceData.TraceSocket_Start : TraceData.TraceSocket_End;
	if (!TraceData.SourceMeshComponent.IsValid() || (TraceData.TraceObject == EGASC_MeleeTrace_TraceObject::Weapon && !TraceData.WeaponAttachmentSocket.IsNone()))
		TraceData.SourceMeshComponent = GetMeshComponent(Instigator, TraceData);
	UMeshComponent* Mesh = TraceData.SourceMeshComponent.Get();
	if (!Mesh || !HasTraceInstance(Mesh, TraceData.MeshInstanceIndex) || !Mesh->DoesSocketExist(TraceData.TraceSocket_Start) || !Mesh->DoesSocketExist(TraceData.TraceSocket_End)
		|| (TraceData.UsesOrientationSocket() && !Mesh->DoesSocketExist(TraceData.OrientationSocket)))
	{
		UE_LOG(LOG_GASC_MeleeTraceSubsystem, Warning, TEXT("Skipping invalid melee shape %s on %s: missing or ambiguous mesh/socket. Attachment=%s Mesh=%s Start=%s End=%s Orientation=%s"),
			*TraceData.ShapeName.ToString(), *GetNameSafe(Instigator), *TraceData.WeaponAttachmentSocket.ToString(), *GetNameSafe(Mesh), *TraceData.TraceSocket_Start.ToString(),
			*TraceData.TraceSocket_End.ToString(), *(TraceData.UsesOrientationSocket() ? TraceData.OrientationSocket.ToString() : FString(TEXT("Unused"))));
		return;
	}

	TraceData.TraceCollisionShape = TraceData.TraceShape->CreateCollisionShape();
	const FCollisionShape& Shape = TraceData.TraceCollisionShape;
	const bool bValidSize = Shape.IsSphere() ? Shape.GetSphereRadius() > 0 :
		Shape.IsCapsule() ? Shape.GetCapsuleRadius() > 0 && Shape.GetCapsuleHalfHeight() >= Shape.GetCapsuleRadius() :
		Shape.IsBox() && Shape.GetExtent().GetMin() > 0;
	const FVector Dimensions = Shape.IsSphere() ? FVector(Shape.GetSphereRadius()) :
		Shape.IsCapsule() ? FVector(Shape.GetCapsuleRadius(), Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight()) : Shape.GetExtent();
	if (!bValidSize || Dimensions.ContainsNaN() || TraceData.LocalOffset.ContainsNaN() || TraceData.RotationOffset.ContainsNaN())
	{
		UE_LOG(LOG_GASC_MeleeTraceSubsystem, Warning, TEXT("Skipping melee shape %s: invalid dimensions/offset."), *TraceData.ShapeName.ToString());
		return;
	}

	FGASC_MeleeTraceWindow* Window = Windows.Find(TraceId);
	if (!Window)
	{
		FGASC_MeleeTraceWindow NewWindow;
		NewWindow.Owner = Instigator;
		NewWindow.Policy = TraceData.HitPolicy == EGASC_MeleeHitPolicy::UseProjectDefault ? MeleeTraceSettings->DefaultHitPolicy : TraceData.HitPolicy;
		if (NewWindow.Policy == EGASC_MeleeHitPolicy::UseProjectDefault) NewWindow.Policy = EGASC_MeleeHitPolicy::OncePerTarget;
		NewWindow.RehitDelay = TraceData.HitCooldownTime < 0 ? MeleeTraceSettings->DefaultRehitDelay : TraceData.HitCooldownTime;
		Window = &Windows.Add(TraceId, MoveTemp(NewWindow));
	}
	TraceData.TraceId = TraceId;
	TraceData.InstigatorActor = Instigator;
	TraceData.HitPolicy = Window->Policy;
	TraceData.HitCooldownTime = FMath::Max(0.f, Window->RehitDelay);
	TraceData.Rank = TraceData.Rank < 0 ? FMath::Max(0, MeleeTraceSettings->DefaultRank) : TraceData.Rank;
	TraceData.Group = TraceData.Group < 0 ? FMath::Max(0, MeleeTraceSettings->DefaultGroup) : TraceData.Group;
	TraceData.TraceDensity = FMath::Clamp(TraceData.TraceDensity, 1, 4096);
	TraceData.ShapeIndex = 0;
	for (const auto& Other : MeleeTraceRequests)
		if (Other.TraceId == TraceId) TraceData.ShapeIndex = FMath::Max(TraceData.ShapeIndex, Other.ShapeIndex + 1);
	if (TraceData.ShapeName.IsNone()) TraceData.ShapeName = FName(*FString::Printf(TEXT("Shape %d"), TraceData.ShapeIndex));
	TraceData.SwingStartTime = GetWorld()->GetTimeSeconds();
	TraceData.PreviousSocketTransform = GetTraceSocketTransform(Mesh, TraceData.TraceSocket_Start, TraceData.MeshInstanceIndex);
	TraceData.PreviousSocketTransform.SetScale3D(FVector::OneVector);
	TraceData.PreviousEndLocal = TraceData.PreviousSocketTransform.InverseTransformPositionNoScale(GetTraceSocketTransform(Mesh, TraceData.TraceSocket_End, TraceData.MeshInstanceIndex).GetLocation());
	TraceData.PreviousOrientationLocal = TraceData.UsesOrientationSocket()
		? TraceData.PreviousSocketTransform.InverseTransformPositionNoScale(GetTraceSocketTransform(Mesh, TraceData.OrientationSocket, TraceData.MeshInstanceIndex).GetLocation()) : FVector::ZeroVector;
	TraceData.HitActors.Reset();
	TraceData.HitActors_PreviousFrames.Reset();
	TraceData.HitResults_PreviousFrames.Reset();
	TraceData.PerActorHitStamps.Reset();
	GetTraceSamples(Mesh, TraceData.TraceDensity, TraceData.TraceSocket_Start, TraceData.TraceSocket_End, TraceData.PreviousFrameSamples, TraceData.MeshInstanceIndex);
	MeleeTraceRequests.Add(MoveTemp(TraceData));
}

bool UGASC_MeleeTrace_Subsystem::IsMeleeTraceInProgress(FGuid TraceId)
{
	const auto* Window = Windows.Find(TraceId);
	return Window && !Window->bClosing;
}

bool UGASC_MeleeTrace_Subsystem::CancelMeleeTrace(FGuid TraceId)
{
	if (auto* Window = Windows.Find(TraceId))
	{
		// Preserve the final pose until the post-animation sweep. Never remove a sibling window.
		Window->bClosing = true;
		if (bProcessing) CanceledDuringDispatch.Add(TraceId);
		if (GetWorld()->WorldType == EWorldType::EditorPreview)
		{
			LiveDebugSamples.RemoveAll([TraceId](const auto& Sample) { return Sample.WindowId == TraceId; });
			if (!bProcessing)
			{
				if (auto* Weapon = Window->PreviewWeapon.Get()) Weapon->DestroyComponent();
				MeleeTraceRequests.RemoveAll([TraceId](const auto& Trace) { return Trace.TraceId == TraceId; });
				Windows.Remove(TraceId);
			}
		}
		return true;
	}
	return false;
}

void UGASC_MeleeTrace_Subsystem::SetNotifyPreviewContext(FGuid WindowId, USkeletalMeshComponent* Mesh,
	const UObject* Animation, float Start, float End)
{
	if (GetWorld()->WorldType != EWorldType::EditorPreview) return;
	if (auto* Window = Windows.Find(WindowId))
	{
		Window->bNotifyPreview = true;
		Window->PreviewMesh = Mesh;
		Window->PreviewAnimation = Animation;
		Window->PreviewStart = Start;
		Window->PreviewEnd = End;
	}
}

void UGASC_MeleeTrace_Subsystem::SetNotifyPreviewWeapon(FGuid WindowId, UMeshComponent* Weapon)
{
	if (!Weapon || GetWorld()->WorldType != EWorldType::EditorPreview) return;
	// An authoring attachment must survive incomplete/invalid trace rows. The notify
	// context and end callback still bound its lifetime, even with no collision requests.
	auto& Window = Windows.FindOrAdd(WindowId);
	Window.Owner = Weapon->GetOwner();
	Window.PreviewWeapon = Weapon;
}

void UGASC_MeleeTrace_Subsystem::PrunePreviewWindows()
{
	TArray<FGuid> Expired;
	for (const auto& Pair : Windows)
	{
		const auto& Window = Pair.Value;
		if (!Window.bNotifyPreview) continue;
		const auto* Mesh = Window.PreviewMesh.Get();
		auto* Preview = Mesh ? Cast<UAnimSingleNodeInstance>(Mesh->GetAnimInstance()) : nullptr;
		bool bInside = false;
		if (Preview && Window.PreviewAnimation.IsValid())
		{
			const float Time = Preview->GetCurrentTime();
			auto IsInside = [&Window](float Position) { return Position >= Window.PreviewStart && Position < Window.PreviewEnd; };
			if (Preview->GetCurrentAsset() == Window.PreviewAnimation.Get()) bInside = IsInside(Time);
			else if (const auto* Montage = Cast<UAnimMontage>(Preview->GetCurrentAsset()))
				for (const auto& Slot : Montage->SlotAnimTracks)
					if (const auto* Segment = Slot.AnimTrack.GetSegmentAtTime(Time))
						if (Segment->GetAnimReference() == Window.PreviewAnimation.Get())
							bInside |= IsInside(Segment->ConvertTrackPosToAnimPos(Time));
		}
		if (!bInside || Window.bClosing) Expired.Add(Pair.Key);
	}
	for (const FGuid Id : Expired) CancelMeleeTrace(Id);
}

UMeshComponent* UGASC_MeleeTrace_Subsystem::GetNotifyPreviewWeapon(FGuid WindowId) const
{
	const auto* Window = Windows.Find(WindowId);
	return Window ? Window->PreviewWeapon.Get() : nullptr;
}

void UGASC_MeleeTrace_Subsystem::SynchronizePreviewAttachments()
{
#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::EditorPreview) return;
	// Persona may seek without notify callbacks, or remove attached components while
	// refreshing the same asset. Use the evaluated playhead to restore active previews.
	for (TActorIterator<AActor> Actor(GetWorld()); Actor; ++Actor)
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (auto* Mesh : Meshes)
		{
			auto* Preview = Cast<UAnimSingleNodeInstance>(Mesh->GetAnimInstance());
			if (!Preview) continue;
			auto EnsureAtTime = [Mesh](UAnimSequenceBase* Animation, float Time)
			{
				if (!Animation) return;
				for (const auto& Event : Animation->Notifies)
					if (Time >= Event.GetTime() && Time < Event.GetTime() + Event.GetDuration())
						if (auto* Notify = Cast<UGASC_MeleeTrace_NotifyState>(Event.NotifyStateClass))
							if (Notify->ShouldFireInEditor()) Notify->EnsurePreviewWindow(Mesh, Animation, Event);
			};
			const float Time = Preview->GetCurrentTime();
			EnsureAtTime(Cast<UAnimSequenceBase>(Preview->GetCurrentAsset()), Time);
			if (auto* Montage = Cast<UAnimMontage>(Preview->GetCurrentAsset()))
				for (const auto& Slot : Montage->SlotAnimTracks)
					if (const auto* Segment = Slot.AnimTrack.GetSegmentAtTime(Time))
						EnsureAtTime(Cast<UAnimSequenceBase>(Segment->GetAnimReference()), Segment->ConvertTrackPosToAnimPos(Time));
		}
	}
#endif
}

void UGASC_MeleeTrace_Subsystem::ProcessMeleeTraces(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProcessMeleeTraces);
	if (bProcessing) return;
	TGuardValue<bool> ProcessingGuard(bProcessing, true);
	CanceledDuringDispatch.Reset();
	UWorld* World = GetWorld();
	const double Now = World->GetTimeSeconds();
	const FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(MeleeTraceSettings->CollisionObjectTypes);
#if !UE_BUILD_SHIPPING
	const bool bDraw = DebugOptions.bEnabled || GASCourse_MeleeSubSystemCVars::CvarEnableMeleeTracesDebug.GetValueOnGameThread();
#endif
	struct FCandidate
	{
		int32 RequestIndex;
		FHitResult Hit;
		float Time;
		int32 DebugIndex;
	};
	TArray<FCandidate> Candidates;
	TArray<FGASC_MeleeDebugSample> FrameSamples;
#if !UE_BUILD_SHIPPING
	TArray<AActor*> FrameActors;
#endif
	// Each window/group/target selects one winner across ALL shapes and substeps before gameplay callbacks.
	TMap<FGuid, TMap<int32, TMap<TWeakObjectPtr<AActor>, int32>>> Selected;

	for (int32 RequestIndex = 0; RequestIndex < MeleeTraceRequests.Num(); ++RequestIndex)
	{
		auto& Trace = MeleeTraceRequests[RequestIndex];
		auto* Window = Windows.Find(Trace.TraceId);
		UMeshComponent* Mesh = Trace.SourceMeshComponent.Get();
		if (!Window || !IsValid(Trace.InstigatorActor) || !Mesh || !HasTraceInstance(Mesh, Trace.MeshInstanceIndex)) continue;
#if !UE_BUILD_SHIPPING
        if (DebugOptions.bRecord && DebugOptions.bCaptureActors && FrameActors.Num() < 17) FrameActors.AddUnique(Trace.InstigatorActor);
#endif
		FTransform CurrentSocket = GetTraceSocketTransform(Mesh, Trace.TraceSocket_Start, Trace.MeshInstanceIndex);
		CurrentSocket.SetScale3D(FVector::OneVector);
		const FVector CurrentEndLocal = CurrentSocket.InverseTransformPositionNoScale(GetTraceSocketTransform(Mesh, Trace.TraceSocket_End, Trace.MeshInstanceIndex).GetLocation());
		const bool bAim = Trace.UsesOrientationSocket();
		const FVector CurrentOrientationLocal = bAim
			? CurrentSocket.InverseTransformPositionNoScale(GetTraceSocketTransform(Mesh, Trace.OrientationSocket, Trace.MeshInstanceIndex).GetLocation()) : FVector::ZeroVector;
		if (CurrentSocket.ContainsNaN() || CurrentEndLocal.ContainsNaN() || CurrentOrientationLocal.ContainsNaN()) continue;
		const auto& Shape = Trace.TraceCollisionShape;
		const float SmallestExtent = Shape.IsSphere() ? Shape.GetSphereRadius() :
			Shape.IsCapsule() ? Shape.GetCapsuleRadius() : Shape.GetExtent().GetMin();
		const FVector AnchorOffset = GASC_MeleeTrace::ShapeAnchorOffset(Shape, Trace.ShapeAnchor);
		const float ShapeReach = (Shape.IsSphere() ? Shape.GetSphereRadius() :
			Shape.IsCapsule() ? Shape.GetCapsuleHalfHeight() : Shape.GetExtent().Size()) + AnchorOffset.Size();
		const float SegmentLength = FMath::Max(Trace.PreviousEndLocal.Size(), CurrentEndLocal.Size());
		int32 Density = Trace.TraceDensity;
		if (MeleeTraceSettings->bAutoTraceDensity)
			Density = FMath::Max(Density, FMath::CeilToInt(SegmentLength / FMath::Max(0.1f, SmallestExtent)));
		// No spatial cap: skipping samples would leave holes along the weapon.
		const float SocketAngle = Trace.PreviousSocketTransform.GetRotation().AngularDistance(CurrentSocket.GetRotation());
		// A stationary single-socket shape has no segment rotation. Dotting two zero
		// directions incorrectly reports 90 degrees and creates unnecessary substeps.
		const FVector PreviousDirection = bAim ? Trace.PreviousOrientationLocal : Trace.PreviousEndLocal;
		const FVector CurrentDirection = bAim ? CurrentOrientationLocal : CurrentEndLocal;
		const FQuat PreviousAlignment = PreviousDirection.IsNearlyZero() ? FQuat::Identity : FRotationMatrix::MakeFromZ(PreviousDirection).ToQuat();
		const FQuat CurrentAlignment = CurrentDirection.IsNearlyZero() ? FQuat::Identity : FRotationMatrix::MakeFromZ(CurrentDirection).ToQuat();
		const float SegmentAngle = PreviousAlignment.AngularDistance(CurrentAlignment);
		const float Angle = SocketAngle + (Trace.bAlignToSocketSegment || bAim ? SegmentAngle : 0.f);
		const float Travel = FVector::Distance(Trace.PreviousSocketTransform.GetLocation(), CurrentSocket.GetLocation()) +
			SocketAngle * (SegmentLength + Trace.LocalOffset.Size() + ShapeReach) +
			FVector::Distance(Trace.PreviousEndLocal, CurrentEndLocal) + SegmentAngle * ShapeReach;
		const float StepDistance = FMath::Max(0.1f, FMath::Min(MeleeTraceSettings->MaxInterpolationDistance, SmallestExtent * 0.5f));
		const int32 DesiredSteps = FMath::Max3(1, FMath::CeilToInt(Travel / StepDistance),
			FMath::CeilToInt(FMath::RadiansToDegrees(Angle) / FMath::Max(0.1f, MeleeTraceSettings->MaxInterpolationAngle)));
		const int32 Steps = FMath::Min(DesiredSteps, FMath::Clamp(MeleeTraceSettings->MaxInterpolationSteps, 1, 4096));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeTrace), false);
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bReturnFaceIndex = true;
		QueryParams.bFindInitialOverlaps = true;
		QueryParams.AddIgnoredActor(Trace.InstigatorActor);
		QueryParams.AddIgnoredActor(Mesh->GetOwner());
		TArray<AActor*> AttachedActors;
		Trace.InstigatorActor->GetAttachedActors(AttachedActors, true, true);
		QueryParams.AddIgnoredActors(AttachedActors);

		const FQuat Offset = Trace.RotationOffset.Quaternion() * Trace.TraceShape->GetRotationOffset();
		auto ShapeRotation = [&Trace, &Offset, bAim, &PreviousAlignment, &CurrentAlignment](const FTransform& Socket, const FVector& EndLocal, float Alpha)
		{
			// Slerp the aim orientation so opposite directions do not collapse through zero.
			const FQuat LocalAlignment = bAim ? FQuat::Slerp(PreviousAlignment, CurrentAlignment, Alpha).GetNormalized()
				: Trace.bAlignToSocketSegment && !EndLocal.IsNearlyZero() ? FRotationMatrix::MakeFromZ(EndLocal).ToQuat() : FQuat::Identity;
			return (Socket.GetRotation() * LocalAlignment * Offset).GetNormalized();
		};
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float AlphaA = float(Step) / Steps;
			const float AlphaB = float(Step + 1) / Steps;
			const FTransform SocketA = GASC_MeleeTrace::InterpolateSocket(Trace.PreviousSocketTransform, CurrentSocket, AlphaA);
			const FTransform SocketB = GASC_MeleeTrace::InterpolateSocket(Trace.PreviousSocketTransform, CurrentSocket, AlphaB);
			const FVector EndA = FMath::Lerp(Trace.PreviousEndLocal, CurrentEndLocal, AlphaA);
			const FVector EndB = FMath::Lerp(Trace.PreviousEndLocal, CurrentEndLocal, AlphaB);
			const FQuat RotationA = ShapeRotation(SocketA, EndA, AlphaA);
			const FQuat RotationB = ShapeRotation(SocketB, EndB, AlphaB);
			FCollisionShape QueryShape = Shape;
			if (MeleeTraceSettings->bConservativeRotationCoverage)
			{
				const float HalfSocketStepAngle = SocketAngle / (2.f * Steps);
				const float ArcPadding = (SegmentLength + Trace.LocalOffset.Size() + AnchorOffset.Size()) * (1.f - FMath::Cos(HalfSocketStepAngle)) +
					FVector::Distance(EndA, EndB) * FMath::Sin(HalfSocketStepAngle);
				const float RotationPadding = Shape.IsSphere() && AnchorOffset.IsNearlyZero() ? 0.f : 2.f * ShapeReach * FMath::Sin(RotationA.AngularDistance(RotationB) * 0.5f);
				const float Padding = ArcPadding + RotationPadding;
				if (Shape.IsSphere()) QueryShape = FCollisionShape::MakeSphere(Shape.GetSphereRadius() + Padding);
				else if (Shape.IsCapsule()) QueryShape = FCollisionShape::MakeCapsule(Shape.GetCapsuleRadius() + Padding, Shape.GetCapsuleHalfHeight() + Padding);
				else QueryShape = FCollisionShape::MakeBox(Shape.GetExtent() + FVector(Padding));
			}
			const int32 SampleCount = SegmentLength <= KINDA_SMALL_NUMBER ? 1 : Density + 1;
			const int32 DrawLimit = FMath::Clamp(MeleeTraceSettings->MaxDebugDrawSamples, 1, 4096);
			const int32 ShapeDrawBudget = FMath::Max(1, DrawLimit / FMath::Max(1, MeleeTraceRequests.Num()));
			const int64 SampleTotal = int64(Steps) * SampleCount;
			const int64 DrawStride = FMath::Max<int64>(1, (SampleTotal + ShapeDrawBudget - 1) / ShapeDrawBudget);
			for (int32 Sample = 0; Sample < SampleCount; ++Sample)
			{
				const float SegmentAlpha = float(Sample) / Density;
				const FVector Start = GASC_MeleeTrace::SampleSocketSegment(SocketA, EndA, Trace.LocalOffset, SegmentAlpha) + RotationA.RotateVector(AnchorOffset);
				const FVector End = GASC_MeleeTrace::SampleSocketSegment(SocketB, EndB, Trace.LocalOffset, SegmentAlpha) + RotationB.RotateVector(AnchorOffset);
				TArray<FHitResult> Hits;
				// UE sweeps translate at a fixed orientation. Test both ends of each angular substep,
				// including stationary rotations; rank/group reduction collapses these duplicate contacts.
				World->SweepMultiByObjectType(Hits, Start, End, RotationB, ObjectParams, QueryShape, QueryParams);
				if (!Shape.IsSphere() && !RotationA.Equals(RotationB, KINDA_SMALL_NUMBER))
				{
					TArray<FHitResult> RotationHits;
					World->SweepMultiByObjectType(RotationHits, Start, End, RotationA, ObjectParams, QueryShape, QueryParams);
					Hits.Append(RotationHits);
				}
				int32 DebugIndex = INDEX_NONE;
#if !UE_BUILD_SHIPPING
				const int64 SampleOrdinal = int64(Step) * SampleCount + Sample;
				const bool bCaptureLive = bDraw && !DebugOptions.bPlayback && !DebugOptions.HiddenActors.Contains(Trace.InstigatorActor)
					&& (SampleOrdinal % DrawStride == 0 || SampleOrdinal + 1 == SampleTotal);
				const int32 CaptureLimit = DebugOptions.bRecord ? FMath::Max(1, MeleeTraceSettings->MaxHistorySamples) : DrawLimit;
				if ((bCaptureLive || DebugOptions.bRecord) && FrameSamples.Num() < CaptureLimit)
				{
					auto& Debug = FrameSamples.AddDefaulted_GetRef();
					DebugIndex = FrameSamples.Num() - 1;
					Debug.Time = Now;
					Debug.FrameStartTime = FMath::Max(double(Trace.SwingStartTime), Now - DeltaTime);
#if OBJECT_TRACE_ENABLED
					Debug.RecordingTime = FObjectTrace::GetWorldElapsedTime(World);
					Debug.RecordingFrameStartTime = FMath::Min(LastRecordingTime, Debug.RecordingTime);
#endif
					Debug.Frame = GFrameCounter;
					Debug.Actor = Trace.InstigatorActor;
					Debug.ActorName = Trace.InstigatorActor->GetName();
					Debug.ShapeName = Trace.ShapeName;
					Debug.WindowId = Trace.TraceId;
					Debug.ShapeIndex = Trace.ShapeIndex;
					Debug.Rank = Trace.Rank;
					Debug.Group = Trace.Group;
					Debug.Policy = Trace.HitPolicy;
					Debug.RehitDelay = Trace.HitCooldownTime;
					Debug.Shape = QueryShape;
					Debug.Start = FTransform(RotationA, Start);
					Debug.End = FTransform(RotationB, End);
					Debug.bInterpolated = Step + 1 < Steps;
					Debug.bBudgetLimited = Steps < DesiredSteps;
					Debug.bHit = !Hits.IsEmpty();
					for (const auto& Hit : Hits)
                    {
                        Debug.ImpactPoints.Add(Hit.ImpactPoint);
                        if (DebugOptions.bRecord && DebugOptions.bCaptureActors && IsValid(Hit.GetActor()) && FrameActors.Num() < 17) FrameActors.AddUnique(Hit.GetActor());
                    }
				}
				else if (DebugOptions.bRecord) ++DroppedDebugSamples;
#endif
				for (const FHitResult& Hit : Hits)
				{
					AActor* Target = Hit.GetActor();
					if (!IsValid(Target) || !Window->Ledger.CanHit(Window->Policy, Trace.Group, Target, Now, Window->RehitDelay)) continue;
					if (MeleeTraceSettings->bCheckObstacles && !Trace.bIgnoreObstacles)
					{
						FHitResult Obstacle;
						FCollisionQueryParams ObstacleParams = QueryParams;
						ObstacleParams.AddIgnoredActor(Target);
						if (World->LineTraceSingleByChannel(Obstacle, Trace.InstigatorActor->GetActorLocation(),
							Hit.ImpactPoint, MeleeTraceSettings->ObstacleChannel, ObstacleParams)) continue;
					}
					const float HitTime = FMath::Lerp(AlphaA, AlphaB, Hit.Time);
					auto& Targets = Selected.FindOrAdd(Trace.TraceId).FindOrAdd(Trace.Group);
					if (int32* Index = Targets.Find(Target))
					{
						FCandidate& Existing = Candidates[*Index];
						const auto& Other = MeleeTraceRequests[Existing.RequestIndex];
						if (GASC_MeleeTrace::PreferHit(Trace.Rank, HitTime, Trace.ShapeIndex, Other.Rank, Existing.Time, Other.ShapeIndex))
							Existing = { RequestIndex, Hit, HitTime, DebugIndex };
					}
					else
					{
						Targets.Add(Target, Candidates.Add({ RequestIndex, Hit, HitTime, DebugIndex }));
					}
				}
			}
		}
		// Advance every shape independently. Never sweep from the beginning of the swing again.
		Trace.PreviousSocketTransform = CurrentSocket;
		Trace.PreviousEndLocal = CurrentEndLocal;
		Trace.PreviousOrientationLocal = CurrentOrientationLocal;
		GetTraceSamples(Mesh, Trace.TraceDensity, Trace.TraceSocket_Start, Trace.TraceSocket_End, Trace.PreviousFrameSamples, Trace.MeshInstanceIndex);
	}

	// Earliest winning contact is used by FirstHitOnly; stable tie-breaks don't depend on map iteration.
	Candidates.Sort([this](const FCandidate& A, const FCandidate& B)
	{
		if (A.Time != B.Time) return A.Time < B.Time;
		const auto& ShapeA = MeleeTraceRequests[A.RequestIndex];
		const auto& ShapeB = MeleeTraceRequests[B.RequestIndex];
		if (ShapeA.Rank != ShapeB.Rank) return ShapeA.Rank < ShapeB.Rank;
		if (A.RequestIndex != B.RequestIndex) return A.RequestIndex < B.RequestIndex;
		return GetNameSafe(A.Hit.GetActor()) < GetNameSafe(B.Hit.GetActor());
	});
	struct FPendingHit { FGASC_MeleeTrace_Subsystem_Data Trace; FHitResult Hit; };
	TArray<FPendingHit> Pending;
	for (const auto& Candidate : Candidates)
	{
		const auto& Trace = MeleeTraceRequests[Candidate.RequestIndex];
		auto* Window = Windows.Find(Trace.TraceId);
		if (!Window || !Window->Ledger.CanHit(Window->Policy, Trace.Group, Candidate.Hit.GetActor(), Now, Window->RehitDelay)) continue;
		Window->Ledger.Record(Trace.Group, Candidate.Hit.GetActor(), Now);
		if (Window->Policy == EGASC_MeleeHitPolicy::FirstHitOnly) Window->bClosing = true;
		if (FrameSamples.IsValidIndex(Candidate.DebugIndex)) FrameSamples[Candidate.DebugIndex].bAccepted = true;
		if (World->IsGameWorld() && (!MeleeTraceSettings->bAuthorityOnly || Trace.InstigatorActor->HasAuthority()))
			Pending.Add({Trace, Candidate.Hit});
	}
#if !UE_BUILD_SHIPPING
	if (bDraw && !DebugOptions.bPlayback) UpdateLiveDebugSamples(FrameSamples);
	if (DebugOptions.bRecord)
	{
		const int32 Limit = FMath::Max(1, MeleeTraceSettings->MaxHistorySamples);
		if (FrameSamples.Num() > Limit) FrameSamples.RemoveAt(0, FrameSamples.Num() - Limit, EAllowShrinking::No);
		const int32 Excess = FMath::Max(0, DebugHistory.Num() + FrameSamples.Num() - Limit);
		if (Excess) DebugHistory.RemoveAt(0, Excess, EAllowShrinking::No);
		CaptureDebugActors(FrameSamples, FrameActors);
		DebugHistory.Append(MoveTemp(FrameSamples));
		PruneDebugActors();
	}
#endif
	// Callbacks may cancel a window or register another attack. No array/map references cross them.
	for (const auto& Hit : Pending)
		if (!CanceledDuringDispatch.Contains(Hit.Trace.TraceId)) DispatchHit(Hit.Trace, Hit.Hit);
	MeleeTraceRequests.RemoveAll([this](const auto& Trace)
	{
		const auto* Window = Windows.Find(Trace.TraceId);
		return !Window || Window->bClosing || !IsValid(Trace.InstigatorActor) || !Trace.SourceMeshComponent.IsValid();
	});
	for (auto It = Windows.CreateIterator(); It; ++It)
		if (!MeleeTraceRequests.ContainsByPredicate([&It](const auto& Trace) { return Trace.TraceId == It.Key(); })
			&& !(It.Value().bNotifyPreview && !It.Value().bClosing && It.Value().Owner.IsValid()
				&& It.Value().PreviewMesh.IsValid() && It.Value().PreviewWeapon.IsValid()))
		{
			if (auto* Weapon = It.Value().PreviewWeapon.Get()) Weapon->DestroyComponent();
			It.RemoveCurrent();
		}
}

void UGASC_MeleeTrace_Subsystem::DispatchHit(const FGASC_MeleeTrace_Subsystem_Data& Trace, const FHitResult& Hit)
{
	AActor* Target = Hit.GetActor();
	if (!IsValid(Target) || !IsValid(Trace.InstigatorActor)) return;
	OnMeleeTraceHit.Broadcast(Trace.TraceId, Trace.ShapeName, Trace.Rank, Trace.Group, Hit, Trace.HitContextTags);
	auto* InstigatorCharacter = Cast<AGASCourseCharacter>(Trace.InstigatorActor);
	auto* TargetCharacter = Cast<AGASCourseCharacter>(Hit.GetActor());
	if (!IsValid(InstigatorCharacter) || !IsValid(TargetCharacter)) return;
	TWeakObjectPtr<UGASCourseAbilitySystemComponent> InstigatorASC = Cast<UGASCourseAbilitySystemComponent>(InstigatorCharacter->GetAbilitySystemComponent());
	TWeakObjectPtr<UGASCourseAbilitySystemComponent> TargetASC = Cast<UGASCourseAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent());
	if (!InstigatorASC.IsValid() || !TargetASC.IsValid()) return;
	if (auto* Pipeline = GetWorld()->GetSubsystem<UGASC_ResourcePipelineSubsystem>())
	{
		FHitContext Context;
		Context.HitTarget = TargetCharacter;
		Context.HitInstigator = InstigatorCharacter;
		Context.OptionalSourceObject = Trace.SourceMeshComponent.IsValid() ? Trace.SourceMeshComponent->GetOwner() : nullptr;
		Context.HitTargetTagsContainer = &TargetASC->GetOwnedGameplayTags();
		Context.HitInstigatorTagsContainer = &InstigatorASC->GetOwnedGameplayTags();
		Context.SetOwnedHitContextTags(Trace.HitContextTags);
		Context.HitResult = Hit;
		Context.HitTimeStamp = GetWorld()->GetTimeSeconds();
		Pipeline->OnHitEvent(Context);
	}
	if (!InstigatorASC.IsValid() || !TargetASC.IsValid()) return;
	FGameplayEventData Payload;
	Payload.Instigator = InstigatorCharacter;
	Payload.Target = TargetCharacter;
	Payload.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));
	Payload.InstigatorTags.AppendTags(InstigatorASC->GetOwnedGameplayTags());
	Payload.InstigatorTags.AppendTags(Trace.HitContextTags);
	Payload.TargetTags.AppendTags(TargetASC->GetOwnedGameplayTags());
	InstigatorASC->HandleGameplayEvent(Event_Gameplay_OnHit, &Payload);
	if (TargetASC.IsValid())
	{
		Payload.EventTag = Event_Gameplay_Reaction_OnHit;
		Payload.InstigatorTags.AddTag(Reaction_OnHit);
		TargetASC->SendGameplayEventAsync(Event_Gameplay_OnHit, Payload);
	}
}

FCollisionObjectQueryParams UGASC_MeleeTrace_Subsystem::ConfigureCollisionObjectParams(
	const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes)
{
	TArray<TEnumAsByte<ECollisionChannel>> CollisionObjectTraces;
	CollisionObjectTraces.AddUninitialized(ObjectTypes.Num());

	for (auto Iter = ObjectTypes.CreateConstIterator(); Iter; ++Iter)
	{
		CollisionObjectTraces[Iter.GetIndex()] = UEngineTypes::ConvertToCollisionChannel(*Iter);
	}

	FCollisionObjectQueryParams ObjectParams;
	for (auto Iter = CollisionObjectTraces.CreateConstIterator(); Iter; ++Iter)
	{
		const ECollisionChannel & Channel = (*Iter);
		if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
		else
		{
			UE_LOG(LogBlueprintUserMessages, Warning, TEXT("%d isn't valid object type"), (int32)Channel);
		}
	}

	return ObjectParams;
}

void UGASC_MeleeTrace_Subsystem::GetTraceSamples(const UMeshComponent* MeshComponent, int32 TraceDensity,
                                                 const FName& StartSocketName, const FName& EndSocketName, TArray<FVector>& OutSamples, int32 MeshInstanceIndex)
{
	OutSamples.Reset();
	if (MeshComponent == nullptr || !HasTraceInstance(MeshComponent, MeshInstanceIndex))
		return;
	TraceDensity = FMath::Clamp(TraceDensity, 1, 4096);
	OutSamples.Reset(TraceDensity + 1);
	const FVector StartSampleLocation = GetTraceSocketTransform(MeshComponent, StartSocketName, MeshInstanceIndex).GetLocation();
	const FVector EndSampleLocation = GetTraceSocketTransform(MeshComponent, EndSocketName, MeshInstanceIndex).GetLocation();
	for (int32 Index = 0; Index <= TraceDensity; Index++)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(TraceDensity);
		const FVector Sample = FMath::Lerp(StartSampleLocation, EndSampleLocation, Alpha);
		//UE_LOG(LOG_GASC_MeleeTraceSubsystem, Log, TEXT("Sample: %s"), *Sample.ToString());
		OutSamples.Add(Sample);
	}
}


TWeakObjectPtr<UMeshComponent> UGASC_MeleeTrace_Subsystem::GetMeshComponent(const AActor* Actor, const FGASC_MeleeTrace_Subsystem_Data& Data)
{
	if (!IsValid(Actor)) return nullptr;
	const FName Start = Data.TraceSocket_Start.IsNone() ? FName("Root") : Data.TraceSocket_Start;
	const FName End = Data.SocketMode == EGASC_MeleeSocketMode::SingleSocket || Data.TraceSocket_End.IsNone() ? Start : Data.TraceSocket_End;
	auto Matches = [&](UMeshComponent* Mesh)
	{
		return Mesh && HasTraceInstance(Mesh, Data.MeshInstanceIndex) && Mesh->DoesSocketExist(Start) && Mesh->DoesSocketExist(End) &&
			(!Data.UsesOrientationSocket() || Mesh->DoesSocketExist(Data.OrientationSocket)) &&
			(Data.MeshComponentNameOrTag.IsNone() || Mesh->GetFName() == Data.MeshComponentNameOrTag || Mesh->ComponentHasTag(Data.MeshComponentNameOrTag));
	};
	if (Data.TraceObject == EGASC_MeleeTrace_TraceObject::CharacterMesh)
	{
		if (Data.MeshComponentNameOrTag.IsNone())
			if (const auto* Character = Cast<ACharacter>(Actor))
				if (Matches(Character->GetMesh())) return Character->GetMesh();
		TInlineComponentArray<UMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (auto* Mesh : Meshes) if (Matches(Mesh)) return Mesh;
		return nullptr;
	}
	// An explicit grip must never fall back to the active weapon or the other hand.
	if (!Data.WeaponAttachmentSocket.IsNone())
	{
		UMeshComponent* Match = nullptr;
		TInlineComponentArray<USceneComponent*> Components;
		Actor->GetComponents(Components);
		TArray<USceneComponent*> Pending;
		for (auto* Component : Components)
			if ((Component->IsA<UStaticMeshComponent>() || Component->IsA<USkeletalMeshComponent>()) && Component->DoesSocketExist(Data.WeaponAttachmentSocket))
				for (USceneComponent* Child : Component->GetAttachChildren())
					if (IsValid(Child) && Child->GetAttachSocketName() == Data.WeaponAttachmentSocket) Pending.AddUnique(Child);
		TSet<USceneComponent*> Seen;
		while (!Pending.IsEmpty())
		{
			USceneComponent* Component = Pending.Pop(EAllowShrinking::No);
			if (!IsValid(Component) || Seen.Contains(Component)) continue;
			Seen.Add(Component);
			if (Component->IsA<UStaticMeshComponent>() || Component->IsA<USkeletalMeshComponent>())
				if (auto* Mesh = Cast<UMeshComponent>(Component); Matches(Mesh))
				{
					if (Match && Match != Mesh)
					{
						UE_LOG(LOG_GASC_MeleeTraceSubsystem, Warning, TEXT("Ambiguous melee weapon attachment %s on %s: %s and %s both contain the trace sockets. Set MeshComponentNameOrTag."),
							*Data.WeaponAttachmentSocket.ToString(), *GetNameSafe(Actor), *GetNameSafe(Match), *GetNameSafe(Mesh));
						return nullptr;
					}
					Match = Mesh;
				}
			for (USceneComponent* Child : Component->GetAttachChildren()) if (IsValid(Child)) Pending.Add(Child);
		}
		return Match;
	}
	if (const auto* Player = Cast<AGASCoursePlayerCharacter>(Actor))
	{
		auto* Weapon = Cast<UMeshComponent>(Player->ActiveWeaponMeshComponent());
		if (Matches(Weapon)) return Weapon;
	}
	const auto* Character = Cast<ACharacter>(Actor);
	UMeshComponent* Body = Character ? Character->GetMesh() : Actor->FindComponentByClass<USkeletalMeshComponent>();
	TArray<USceneComponent*> Children;
	if (Body) Body->GetChildrenComponents(true, Children);
	for (auto* Child : Children)
		if (auto* Mesh = Cast<UMeshComponent>(Child))
			if (Matches(Mesh)) return Mesh;
	TArray<AActor*> Attached;
	Actor->GetAttachedActors(Attached, true, true);
	for (auto* AttachedActor : Attached)
	{
		TInlineComponentArray<UMeshComponent*> Meshes;
		AttachedActor->GetComponents(Meshes);
		for (auto* Mesh : Meshes) if (Matches(Mesh)) return Mesh;
	}
	// Weapon lookup deliberately does not fall through to the body mesh.
	return nullptr;
}

FGASC_MeleeTrace_Subsystem_Data UGASC_MeleeTrace_Subsystem::CreateShapeDataFromRow(
	const FGASC_MeleeTrace_TraceShapeData& RowData) const
{
	FGASC_MeleeTrace_Subsystem_Data TraceData;
	TraceData.TraceDensity = RowData.TraceDensity;
	TraceData.TraceSocket_Start = RowData.StartSocket;
	TraceData.TraceSocket_End = RowData.EndSocket;
	TraceData.SocketMode = RowData.SocketMode;
	TraceData.OrientationSocket = RowData.OrientationSocket;
	TraceData.ShapeAnchor = RowData.ShapeAnchor;
	TraceData.TraceObject = RowData.TraceObject;
	TraceData.WeaponAttachmentSocket = RowData.WeaponAttachmentSocket;
	TraceData.MeshInstanceIndex = RowData.MeshInstanceIndex;
	TraceData.ShapeName = RowData.ShapeName;
	TraceData.Rank = RowData.Rank;
	TraceData.Group = RowData.Group;
	TraceData.HitContextTags = RowData.HitContextTags;
	TraceData.LocalOffset = RowData.LocalOffset;
	TraceData.RotationOffset = RowData.RotationOffset;
	TraceData.bAlignToSocketSegment = RowData.bAlignToSocketSegment;
	TraceData.MeshComponentNameOrTag = RowData.MeshComponentNameOrTag;
	TraceData.bIgnoreObstacles = RowData.bIgnoreObstacles;
	TraceData.HitActors.Reset();
	TraceData.PreviousFrameSamples.Reset();
	TraceData.TraceId = FGuid::NewGuid();
	TraceData.InstigatorActor = nullptr;
	TraceData.SourceMeshComponent = nullptr;
	
	switch (RowData.TraceShape)
	{
	case EGASC_MeleeTrace_TraceShape::Box:
		// Handle Box Trace Shape
			TraceData.TraceShape = NewObject<UGASC_MeleeShape_Box>(GetOuter());
		if (UGASC_MeleeShape_Box* Box = Cast<UGASC_MeleeShape_Box>(TraceData.TraceShape))
		{
			Box->BoxExtent = RowData.BoxExtent;
			break;
		}
		else
		{
			break;
		}

	case EGASC_MeleeTrace_TraceShape::Capsule:
		// Handle Capsule Trace Shape
			TraceData.TraceShape = NewObject<UGASC_MeleeShape_Capsule>(GetOuter());
		if (UGASC_MeleeShape_Capsule* Capsule = Cast<UGASC_MeleeShape_Capsule>(TraceData.TraceShape))
		{
			Capsule->Radius = RowData.CapsuleRadius;
			Capsule->HalfHeight = RowData.CapsuleHeight;
			break;
		}
		else
		{
			break;
		}

	case EGASC_MeleeTrace_TraceShape::Sphere:
		// Handle Sphere Trace Shape
			TraceData.TraceShape = NewObject<UGASC_MeleeShape_Sphere>(GetOuter());
		if (UGASC_MeleeShape_Sphere* Sphere = Cast<UGASC_MeleeShape_Sphere>(TraceData.TraceShape))
		{
			Sphere->Radius = RowData.SphereRadius;
			break;
		}
		else
		{
			break;
		}

	default:
		// Fallback case (optional, depending on your enum design)
			UE_LOGFMT(LOG_GASC_MeleeTraceSubsystem, Warning, "Unknown Trace Shape found in {0}", __FUNCTION__);
		break;

	}
	return TraceData;
}




