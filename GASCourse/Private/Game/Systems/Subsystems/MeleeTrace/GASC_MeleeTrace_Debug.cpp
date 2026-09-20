#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Subsystem.h"
#include "DrawDebugHelpers.h"
#include "Game/Systems/Debugging/GASC_RewindGhosts.h"
#include "HAL/PlatformTime.h"
#if !UE_BUILD_SHIPPING
struct FGASC_MeleeActorFrame
{
 uint64 Frame = 0;
 TArray<FGASC_RewindActorPose> Actors;
 SIZE_T Bytes = 0;
 bool Truncated = false;
};
#endif

void UGASC_MeleeTrace_Subsystem::ClearDebugHistory()
{
	ClearDebugGhosts();
#if !UE_BUILD_SHIPPING
	ActorFrames.Reset();
#endif
	ActorHistoryBytes = 0; DroppedActorFrames = 0;
	DebugHistory.Reset();
	DroppedDebugSamples = 0;
	DebugOptions.bPlayback = false;
}

void UGASC_MeleeTrace_Subsystem::DrawDebugSample(UWorld* World, const FGASC_MeleeDebugSample& Sample, float Duration, bool bDrawLabel) const
{
#if !UE_BUILD_SHIPPING
	if (!World || DebugOptions.HiddenActors.Contains(Sample.Actor)) return;
	const auto* Settings = GetDefault<UGASC_MeleeSubsystem_Settings>();
	const FColor Color = Sample.bHit ? Settings->MeleeTraceHitDebugColor :
		Sample.bInterpolated ? Settings->MeleeTraceInterpolatedDebugColor : Settings->MeleeTraceDebugColor;
	const float Lifetime = FMath::Max(0.f, Duration);
	auto DrawShape = [&](const FTransform& Transform)
	{
		const FVector Position = Transform.GetLocation();
		if (Sample.Shape.IsSphere())
			::DrawDebugSphere(World, Position, Sample.Shape.GetSphereRadius(), 8, Color, false, Lifetime);
		else if (Sample.Shape.IsCapsule())
			::DrawDebugCapsule(World, Position, Sample.Shape.GetCapsuleHalfHeight(), Sample.Shape.GetCapsuleRadius(),
				Transform.GetRotation(), Color, false, Lifetime);
		else if (Sample.Shape.IsBox())
			::DrawDebugBox(World, Position, Sample.Shape.GetExtent(), Transform.GetRotation(), Color, false, Lifetime);
	};
	if (DebugOptions.bShapes && (!Sample.bInterpolated || DebugOptions.bInterpolated))
	{
		if (!Sample.Start.Equals(Sample.End)) DrawShape(Sample.Start);
		DrawShape(Sample.End);
	}
	if (DebugOptions.bPaths)
		::DrawDebugLine(World, Sample.Start.GetLocation(), Sample.End.GetLocation(), Color, false, Lifetime);
	if (DebugOptions.bHitPoints)
		for (const FVector& Point : Sample.ImpactPoints)
			::DrawDebugPoint(World, Point, 8.f, Settings->MeleeTraceHitDebugColor, false, Lifetime);
	if (DebugOptions.bLabels && bDrawLabel && !Sample.bInterpolated)
	{
		const FString PolicyName = StaticEnum<EGASC_MeleeHitPolicy>()->GetNameStringByValue(int64(Sample.Policy));
		const FString Label = FString::Printf(TEXT("%s r%d g%d %s%s%s"), *Sample.ShapeName.ToString(),
			Sample.Rank, Sample.Group, *PolicyName, Sample.bAccepted ? TEXT(" HIT") : Sample.bHit ? TEXT(" contact") : TEXT(""),
			Sample.bBudgetLimited ? TEXT(" [step budget]") : TEXT(""));
		::DrawDebugString(World, Sample.End.GetLocation(), Label, nullptr, Color, Lifetime, false);
	}
#endif
}

void UGASC_MeleeTrace_Subsystem::UpdateLiveDebugSamples(const TArray<FGASC_MeleeDebugSample>& Samples)
{
#if !UE_BUILD_SHIPPING
	const int32 Limit = FMath::Clamp(MeleeTraceSettings->MaxDebugDrawSamples, 1, 4096);
	TArray<int32> Visible;
	for (int32 Index = 0; Index < Samples.Num(); ++Index)
		if (!DebugOptions.HiddenActors.Contains(Samples[Index].Actor)) Visible.Add(Index);
	const int32 Count = FMath::Min(Limit, Visible.Num());
	// Spread the draw budget over the full motion, rather than truncating its tail.
	for (int32 Index = 0; Index < Count; ++Index)
		LiveDebugSamples.Add(Samples[Visible[Count > 1 ? int64(Index) * (Visible.Num() - 1) / (Count - 1) : 0]]);
	if (LiveDebugSamples.Num() > Limit)
		LiveDebugSamples.RemoveAt(0, LiveDebugSamples.Num() - Limit, EAllowShrinking::No);
#endif
}

void UGASC_MeleeTrace_Subsystem::DrawDebugSamples(UWorld* World, const TArray<FGASC_MeleeDebugSample>& Samples, uint64 Frame) const
{
#if !UE_BUILD_SHIPPING
	const int32 Limit = FMath::Clamp(MeleeTraceSettings->MaxDebugDrawSamples, 1, 4096);
	TArray<int32> Visible;
	for (int32 Index = 0; Index < Samples.Num(); ++Index)
		if ((Frame == MAX_uint64 || Samples[Index].Frame == Frame) && !DebugOptions.HiddenActors.Contains(Samples[Index].Actor)) Visible.Add(Index);
	const int32 Count = FMath::Min(Limit, Visible.Num());
	TMap<FGuid, TSet<int32>> LabeledShapes;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const auto& Sample = Samples[Visible[Count > 1 ? int64(Index) * (Visible.Num() - 1) / (Count - 1) : 0]];
		auto& Labels = LabeledShapes.FindOrAdd(Sample.WindowId);
		const bool bLabel = !Sample.bInterpolated && !Labels.Contains(Sample.ShapeIndex);
		if (bLabel) Labels.Add(Sample.ShapeIndex);
		// Own persistence in the bounded sample buffer, never in the engine line batcher.
		DrawDebugSample(World, Sample, 0.f, bLabel);
	}
#endif
}

void UGASC_MeleeTrace_Subsystem::DrawDebugHistory(UWorld* World, double Time, bool bRecordingTime) const
{
#if !UE_BUILD_SHIPPING
	// Do not show stale traces in gaps between attacks. Select the recorded frame containing time.
	const FGASC_MeleeDebugSample* FrameSample = DebugHistory.FindByPredicate([Time, bRecordingTime](const auto& Sample)
	{
		return bRecordingTime ? Sample.RecordingFrameStartTime <= Time && Time <= Sample.RecordingTime :
			Sample.FrameStartTime <= Time && Time <= Sample.Time;
	});
	if (!FrameSample) { const_cast<UGASC_MeleeTrace_Subsystem*>(this)->ClearDebugGhosts(); return; }
    TArray<TSharedPtr<FGASC_RewindMeshPose>> Meshes;
    uint64 Key = HashCombine(GetTypeHash(FrameSample->Frame), GetTypeHash(DebugOptions.MaxGhostActors));
    for (const auto& Hidden : DebugOptions.HiddenActors) Key = HashCombine(Key, GetTypeHash(Hidden));
    for (const auto& Recorded : ActorFrames) if (Recorded->Frame == FrameSample->Frame)
    {
        int32 Visible = 0;
        for (const auto& Actor : Recorded->Actors)
        {
            if (DebugOptions.HiddenActors.Contains(Actor.Actor) || Visible++ >= DebugOptions.MaxGhostActors) continue;
            if (DebugOptions.bActorGhosts) Meshes.Append(Actor.Meshes);
            if (DebugOptions.bActorBounds) DrawDebugBox(World, Actor.Transform.TransformPosition(Actor.Center), Actor.Extent * Actor.Transform.GetScale3D().GetAbs(), Actor.Transform.GetRotation(), FColor(180, 120, 10), false, 0.f);
        }
        break;
    }
    if (!Meshes.IsEmpty())
    {
        if (!ActorGhostRenderer.IsValid()) ActorGhostRenderer = MakeShared<FGASC_RewindGhostRenderer>();
        ActorGhostRenderer->Show(World, Key, Meshes, DebugOptions.MaxGhostActors * 8);
    }
    else const_cast<UGASC_MeleeTrace_Subsystem*>(this)->ClearDebugGhosts();
	DrawDebugSamples(World, DebugHistory, FrameSample->Frame);
#endif
}

void UGASC_MeleeTrace_Subsystem::ClearDebugGhosts()
{
#if !UE_BUILD_SHIPPING
 if (ActorGhostRenderer.IsValid()) ActorGhostRenderer->ClearGhostMeshes();
#endif
}
void UGASC_MeleeTrace_Subsystem::PruneDebugActors()
{
#if !UE_BUILD_SHIPPING
 const uint64 First = DebugHistory.IsEmpty() ? MAX_uint64 : DebugHistory[0].Frame;
 while (!ActorFrames.IsEmpty() && (ActorFrames[0]->Frame < First || ActorFrames.Num() > 900 || ActorHistoryBytes > 64ull * 1024 * 1024))
 {
  ActorHistoryBytes -= ActorFrames[0]->Bytes; ActorFrames.RemoveAt(0, 1, EAllowShrinking::No);
 }
#endif
}
void UGASC_MeleeTrace_Subsystem::CaptureDebugActors(const TArray<FGASC_MeleeDebugSample>& Samples, const TArray<AActor*>& Actors)
{
#if !UE_BUILD_SHIPPING
 if (!DebugOptions.bRecord || !DebugOptions.bCaptureActors || Samples.IsEmpty()) return;
 const uint64 Frame = Samples[0].Frame;
 if (!ActorFrames.IsEmpty() && ActorFrames.Last()->Frame == Frame) return;
 const double Start = FPlatformTime::Seconds(), Deadline = Start + FMath::Clamp(DebugOptions.ActorCaptureBudgetMs, .1f, 5.f) / 1000.;
 auto Recorded = MakeShared<FGASC_MeleeActorFrame>(); Recorded->Frame = Frame;
 for (AActor* Actor : Actors)
 {
  if (Recorded->Actors.Num() >= 16 || FPlatformTime::Seconds() > Deadline) { Recorded->Truncated = true; break; }
  auto Pose = GASC_CaptureRewindActor(Actor, Recorded->Truncated, Deadline);
  Recorded->Bytes += Pose.Bytes(); Recorded->Actors.Add(MoveTemp(Pose));
 }
 Recorded->Bytes += sizeof(FGASC_MeleeActorFrame) + Recorded->Actors.GetAllocatedSize();
 if (Recorded->Truncated) ++DroppedActorFrames;
 ActorHistoryBytes += Recorded->Bytes; ActorFrames.Add(Recorded);
 LastActorCaptureMs = (FPlatformTime::Seconds() - Start) * 1000.;
#endif
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR && !UE_BUILD_SHIPPING
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "RenderingThread.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeGhostHistoryTest, "GASCourse.MeleeTrace.ActorRewind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGASC_MeleeGhostHistoryTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { FlushRenderingCommands(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
 auto* System = World->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
 System->DebugOptions.bRecord = true; System->DebugOptions.ActorCaptureBudgetMs = 5.f;
 auto* BodyAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Sword_Animations/Demo/Mannequin_UE4/Character/Mesh/SK_Mannequin.SK_Mannequin"));
 auto* WeaponAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (!TestNotNull(TEXT("Body fixture"), BodyAsset) || !TestNotNull(TEXT("Weapon fixture"), WeaponAsset)) return false;
 AActor* Actor = World->SpawnActor<AActor>();
 auto* Body = NewObject<USkeletalMeshComponent>(Actor); Actor->AddInstanceComponent(Body); Actor->SetRootComponent(Body); Body->SetSkeletalMesh(BodyAsset); Body->RegisterComponent(); Body->RefreshBoneTransforms();
 AActor* Weapon = World->SpawnActor<AActor>();
 auto* Blade = NewObject<UStaticMeshComponent>(Weapon); Weapon->AddInstanceComponent(Blade); Weapon->SetRootComponent(Blade); Blade->SetStaticMesh(WeaponAsset); Blade->RegisterComponent();
 Weapon->AttachToActor(Actor, FAttachmentTransformRules::KeepRelativeTransform); Weapon->SetActorRelativeLocation(FVector(40, 0, 80));
 FGASC_MeleeDebugSample Sample; Sample.Actor = Actor; Sample.Frame = 10; Sample.FrameStartTime = .9; Sample.Time = 1; Sample.Shape = FCollisionShape::MakeSphere(10);
 TArray<FGASC_MeleeDebugSample> Samples = {Sample}; TArray<AActor*> Actors = {Actor};
 System->CaptureDebugActors(Samples, Actors); System->CaptureDebugActors(Samples, Actors); System->DebugHistory = Samples;
 if (!TestEqual(TEXT("Captured once for all sweeps in one frame"), System->ActorFrames.Num(), 1)) return false;
 if (!TestEqual(TEXT("Body and attached weapon are captured"), System->ActorFrames[0]->Actors[0].Meshes.Num(), 2)) return false;
 const auto First = System->ActorFrames[0]; const FVector WeaponLocation = Weapon->GetActorLocation();
 Actor->SetActorLocation(FVector(200, 0, 0)); Weapon->SetActorRelativeLocation(FVector(100, 0, 80));
 Samples[0].Frame = 11; Samples[0].Time = 1.1; Samples[0].FrameStartTime = 1.01;
 System->CaptureDebugActors(Samples, Actors); System->DebugHistory.Append(Samples);
 TestTrue(TEXT("Earlier attached-weapon transform is immutable"), First->Actors[0].Meshes[1]->Transform.GetLocation().Equals(WeaponLocation));
 Actor->Destroy(); Weapon->Destroy(); System->DrawDebugHistory(World, .95);
 if (!TestTrue(TEXT("Historical ghosts survive source destruction"), System->ActorGhostRenderer.IsValid())) return false;
 auto& Renderer = *System->ActorGhostRenderer;
 TestEqual(TEXT("One skeletal body ghost"), Renderer.SkeletalPool.Num(), 1); TestEqual(TEXT("One static weapon ghost"), Renderer.StaticPool.Num(), 1);
 TestTrue(TEXT("Cookable project ghost material"), Renderer.GhostBaseMaterial.IsValid() && Renderer.GhostBaseMaterial->GetPathName().StartsWith(TEXT("/Game/GASCourse/")));
 if (Renderer.StaticPool.Num()) TestTrue(TEXT("Weapon rendered at captured world transform"), Renderer.StaticPool[0]->GetComponentLocation().Equals(WeaponLocation));
 System->DrawDebugHistory(World, 20); TestFalse(TEXT("Gaps clear old ghosts"), Renderer.GhostActor.IsValid());
 System->DebugHistory.RemoveAt(0); System->PruneDebugActors(); TestEqual(TEXT("Trace eviction also evicts actor snapshot"), System->ActorFrames.Num(), 1);
 System->ClearDebugHistory(); TestEqual(TEXT("Clear releases actor memory"), uint64(System->ActorHistoryBytes), uint64(0));
 return true;
}
#endif
