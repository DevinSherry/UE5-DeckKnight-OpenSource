#include "Misc/AutomationTest.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_Subsystem.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Misc/ScopeExit.h"
#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_NotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotifyQueue.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"
#include "Game/Systems/Damage/Pipeline/GASC_ResourcePipelineTypes.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeInstancedWeaponTest, "GASCourse.MeleeTrace.InstancedWeaponSockets",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGASC_MeleeInstancedWeaponTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
 auto* System = World->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
 auto* Actor = World->SpawnActor<AActor>();
 auto* Body = NewObject<USkeletalMeshComponent>(Actor); Actor->AddInstanceComponent(Body); Actor->SetRootComponent(Body);
 auto* SkeletonMesh = NewObject<USkeletalMesh>(Body);
 SkeletonMesh->SetSkeleton(NewObject<USkeleton>(SkeletonMesh));
 {
  FReferenceSkeletonModifier Bones(SkeletonMesh->GetRefSkeleton(), SkeletonMesh->GetSkeleton());
  Bones.Add(FMeshBoneInfo(TEXT("root"), TEXT("root"), INDEX_NONE), FTransform::Identity);
 }
 auto* Grip = NewObject<USkeletalMeshSocket>(SkeletonMesh); Grip->SocketName = TEXT("Hand_RSocket"); Grip->BoneName = TEXT("root");
 SkeletonMesh->GetMeshOnlySocketList().Add(Grip); SkeletonMesh->RebuildSocketMap(); Body->SetSkeletalMeshAsset(SkeletonMesh);
 auto* Weapon = NewObject<UInstancedStaticMeshComponent>(Actor); Actor->AddInstanceComponent(Weapon);
 auto* Mesh = NewObject<UStaticMesh>(Weapon);
 auto* Start = NewObject<UStaticMeshSocket>(Mesh); Start->SocketName = TEXT("WeaponTrace_Start"); Start->RelativeLocation = FVector(10, 0, 0); Mesh->AddSocket(Start);
 auto* End = NewObject<UStaticMeshSocket>(Mesh); End->SocketName = TEXT("WeaponTrace_End"); End->RelativeLocation = FVector(10, 0, 100); Mesh->AddSocket(End);
 Weapon->SetStaticMesh(Mesh); Weapon->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, Grip->SocketName);
 Weapon->SetRelativeTransform(FTransform(FRotator(0, 35, 0), FVector(40, 50, 60)));
 Weapon->AddInstance(FTransform(FVector(500, 0, 0)));
 Weapon->AddInstance(FTransform(FRotator(20, 60, 0), FVector(0, 200, 0), FVector(2)));
 FGASC_MeleeTrace_TraceShapeData Row; Row.TraceShape = EGASC_MeleeTrace_TraceShape::Sphere;
 Row.WeaponAttachmentSocket = Grip->SocketName; Row.MeshInstanceIndex = 1;
 auto Data = System->CreateShapeDataFromRow(Row);
 TestEqual(TEXT("Row preserves instance selection"), Data.MeshInstanceIndex, 1);
 TestTrue(TEXT("Hand_RSocket resolves instanced weapon on skeletal body"), System->GetMeshComponent(Actor, Data).Get() == Weapon);
 FTransform InstanceWorld; Weapon->GetInstanceTransform(1, InstanceWorld, true);
 TArray<FVector> Samples;
 System->GetTraceSamples(Weapon, 2, Start->SocketName, End->SocketName, Samples, 1);
 TestEqual(TEXT("Three segment samples"), Samples.Num(), 3);
 if (Samples.Num() == 3)
 {
  TestTrue(TEXT("Start includes instance translation rotation and scale"), Samples[0].Equals(InstanceWorld.TransformPosition(Start->RelativeLocation)));
  TestTrue(TEXT("End includes instance translation rotation and scale"), Samples[2].Equals(InstanceWorld.TransformPosition(End->RelativeLocation)));
 }
 const FGuid Window = FGuid::NewGuid(); System->RequestMeleeTraceWindow(Actor, {Data}, Window);
 TestEqual(TEXT("Instanced weapon opens trace"), System->GetActiveTraces().Num(), 1);
 if (System->GetActiveTraces().Num() == 1)
  TestTrue(TEXT("Initial trace uses selected instance"), System->GetActiveTraces()[0].PreviousSocketTransform.GetLocation().Equals(InstanceWorld.TransformPosition(Start->RelativeLocation)));
 System->CancelMeleeTrace(Window);
 Data.WeaponAttachmentSocket = TEXT("Hand_LSocket");
 TestFalse(TEXT("Wrong hand does not fall back"), System->GetMeshComponent(Actor, Data).IsValid());
 Data.WeaponAttachmentSocket = Grip->SocketName; Data.MeshInstanceIndex = 2;
 TestFalse(TEXT("Missing instance cannot resolve"), System->GetMeshComponent(Actor, Data).IsValid());
 System->GetTraceSamples(Weapon, 2, Start->SocketName, End->SocketName, Samples, 2);
 TestTrue(TEXT("Missing instance clears samples"), Samples.IsEmpty());
 Weapon->ClearInstances(); Data.MeshInstanceIndex = 0;
 TestFalse(TEXT("Empty component cannot resolve"), System->GetMeshComponent(Actor, Data).IsValid());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeWeaponAttachmentTest, "GASCourse.MeleeTrace.WeaponAttachmentSockets",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGASC_MeleeWeaponAttachmentTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
 auto* System = World->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
 auto* Actor = World->SpawnActor<AActor>();
 auto MakeStatic = [](AActor* Owner, const TArray<FName>& Names)
 {
  auto* Component = NewObject<UStaticMeshComponent>(Owner); Owner->AddInstanceComponent(Component);
  auto* Asset = NewObject<UStaticMesh>(Component);
  for (const FName Name : Names) { auto* Socket = NewObject<UStaticMeshSocket>(Asset); Socket->SocketName = Name; Asset->AddSocket(Socket); }
  Component->SetStaticMesh(Asset); return Component;
 };
 auto* Body = MakeStatic(Actor, {TEXT("LeftGrip"), TEXT("RightGrip"), TEXT("EmptyGrip")}); Actor->SetRootComponent(Body);
 auto* Left = MakeStatic(Actor, {TEXT("Start"), TEXT("End")});
 Left->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("LeftGrip"));
 Left->SetRelativeLocation(FVector(0, -50, 0));
 // An actor with a scene root and a skeletal mesh child, attached to the other hand.
 auto* WeaponActor = World->SpawnActor<AActor>();
 auto* Root = NewObject<USceneComponent>(WeaponActor); WeaponActor->AddInstanceComponent(Root); WeaponActor->SetRootComponent(Root);
 Root->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("RightGrip"));
 auto* Right = NewObject<USkeletalMeshComponent>(WeaponActor); WeaponActor->AddInstanceComponent(Right);
 auto* Asset = NewObject<USkeletalMesh>(Right);
 Asset->SetSkeleton(NewObject<USkeleton>(Asset));
 {
  FReferenceSkeletonModifier Bones(Asset->GetRefSkeleton(), Asset->GetSkeleton());
  Bones.Add(FMeshBoneInfo(TEXT("root"), TEXT("root"), INDEX_NONE), FTransform::Identity);
 }
 for (const FName Name : {FName(TEXT("Start")), FName(TEXT("End"))})
 { auto* Socket = NewObject<USkeletalMeshSocket>(Asset); Socket->SocketName = Name; Socket->BoneName = TEXT("root"); Asset->GetMeshOnlySocketList().Add(Socket); }
 Asset->RebuildSocketMap(); Right->SetSkeletalMeshAsset(Asset);
 Right->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
 FGASC_MeleeTrace_TraceShapeData Row; Row.TraceShape = EGASC_MeleeTrace_TraceShape::Sphere; Row.StartSocket = TEXT("Start"); Row.EndSocket = TEXT("End"); Row.WeaponAttachmentSocket = TEXT("LeftGrip");
 auto Data = System->CreateShapeDataFromRow(Row);
 TestEqual(TEXT("Row preserves attachment selection"), Data.WeaponAttachmentSocket, Row.WeaponAttachmentSocket);
 TestTrue(TEXT("Left hand resolves static weapon"), System->GetMeshComponent(Actor, Data).Get() == Left);
 Data.WeaponAttachmentSocket = TEXT("RightGrip");
 TestTrue(TEXT("Right hand resolves skeletal weapon through actor root"), System->GetMeshComponent(Actor, Data).Get() == Right);
 Data.WeaponAttachmentSocket = TEXT("MissingGrip"); TestFalse(TEXT("Unknown grip never falls back"), System->GetMeshComponent(Actor, Data).IsValid());
 Data.WeaponAttachmentSocket = TEXT("EmptyGrip"); TestFalse(TEXT("Empty grip never falls back"), System->GetMeshComponent(Actor, Data).IsValid());
 Data.WeaponAttachmentSocket = TEXT("LeftGrip"); Data.TraceSocket_End = TEXT("MissingEnd");
 TestFalse(TEXT("Both segment sockets must exist on selected mesh"), System->GetMeshComponent(Actor, Data).IsValid());
 Data.SocketMode = EGASC_MeleeSocketMode::SingleSocket;
 TestTrue(TEXT("Single socket does not require unused end"), System->GetMeshComponent(Actor, Data).Get() == Left);
 Data.OrientationSocket = TEXT("MissingAim"); TestFalse(TEXT("Configured orientation socket is validated"), System->GetMeshComponent(Actor, Data).IsValid());
 Data = System->CreateShapeDataFromRow(Row);
 auto* Duplicate = MakeStatic(Actor, {TEXT("Start"), TEXT("End")});
 Duplicate->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("LeftGrip"));
 TestFalse(TEXT("Ambiguous grip does not pick by component order"), System->GetMeshComponent(Actor, Data).IsValid());
 Left->ComponentTags.Add(TEXT("MainBlade")); Data.MeshComponentNameOrTag = TEXT("MainBlade");
 TestTrue(TEXT("Tag disambiguates matching meshes"), System->GetMeshComponent(Actor, Data).Get() == Left);
 // Explicit attachment selection also wins over a pre-supplied mesh (including notify preview overrides).
 Data.SourceMeshComponent = Right;
 auto RightData = System->CreateShapeDataFromRow(Row); RightData.WeaponAttachmentSocket = TEXT("RightGrip");
 const FGuid Window = FGuid::NewGuid(); System->RequestMeleeTraceWindow(Actor, {Data, RightData}, Window);
 TestTrue(TEXT("Requested trace uses the selected hand"), System->GetActiveTraces().Num() == 2 && System->GetActiveTraces()[0].SourceMeshComponent.Get() == Left && System->GetActiveTraces()[1].SourceMeshComponent.Get() == Right);
 if (!System->GetActiveTraces().IsEmpty()) TestTrue(TEXT("Trace samples start at selected weapon socket"), System->GetActiveTraces()[0].PreviousSocketTransform.GetLocation().Equals(Left->GetSocketLocation(TEXT("Start"))));
 System->CancelMeleeTrace(Window);
 Data.WeaponAttachmentSocket = NAME_None; Data.SourceMeshComponent.Reset();
 // Legacy lookup on an attached actor is still allowed when no grip is authored.
 Data.MeshComponentNameOrTag = NAME_None;
 TestTrue(TEXT("Empty selection retains automatic attached-actor lookup"), System->GetMeshComponent(Actor, Data).Get() == Right);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeHitPolicyTest, "GASCourse.MeleeTrace.HitPolicies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_MeleeHitPolicyTest::RunTest(const FString& Parameters)
{
	FHitContext RetainedContext;
	{
		FHitContext Original;
		Original.SetOwnedHitContextTags(FGameplayTagContainer());
		RetainedContext = Original;
	}
	TestTrue(TEXT("Copied hit contexts retain their authored tag storage"),
		RetainedContext.OwnedHitContextTags.IsValid() && RetainedContext.HitContextTagsContainer == RetainedContext.OwnedHitContextTags.Get());
	AActor* Target = GetMutableDefault<AActor>();
	AActor* OtherTarget = GetMutableDefault<APawn>();
	FGASC_MeleeHitLedger Ledger;
	Ledger.Record(0, Target, 1.0);
	TestFalse(TEXT("Sibling shapes cannot double-hit a target"), Ledger.CanHit(EGASC_MeleeHitPolicy::OncePerTarget, 0, Target, 2.0, 0.2));
	TestTrue(TEXT("Different groups can intentionally hit independently"), Ledger.CanHit(EGASC_MeleeHitPolicy::OncePerTarget, 1, Target, 2.0, 0.2));
	TestTrue(TEXT("OncePerTarget permits another target"), Ledger.CanHit(EGASC_MeleeHitPolicy::OncePerTarget, 0, OtherTarget, 2.0, 0.2));
	TestFalse(TEXT("FirstHitOnly stops other groups and targets"), Ledger.CanHit(EGASC_MeleeHitPolicy::FirstHitOnly, 1, OtherTarget, 2.0, 0.2));
	TestFalse(TEXT("Cooldown blocks early rehit"), Ledger.CanHit(EGASC_MeleeHitPolicy::RehitAfterDelay, 0, Target, 1.25, 0.5));
	TestTrue(TEXT("Cooldown permits exact boundary rehit"), Ledger.CanHit(EGASC_MeleeHitPolicy::RehitAfterDelay, 0, Target, 1.5, 0.5));
	TestFalse(TEXT("Unlimited still deduplicates substeps at the same time"), Ledger.CanHit(EGASC_MeleeHitPolicy::Unlimited, 0, Target, 1.0, 0));
	TestTrue(TEXT("Unlimited permits next update"), Ledger.CanHit(EGASC_MeleeHitPolicy::Unlimited, 0, Target, 1.01, 0));
	TestTrue(TEXT("New windows do not inherit previous hit history"), FGASC_MeleeHitLedger().CanHit(EGASC_MeleeHitPolicy::OncePerTarget, 0, Target, 2.0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeRankTest, "GASCourse.MeleeTrace.RankPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_MeleeRankTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Lower rank wins even if sampled later"), GASC_MeleeTrace::PreferHit(0, 0.9f, 4, 1, 0.1f, 0));
	TestFalse(TEXT("Higher rank cannot steal a hit"), GASC_MeleeTrace::PreferHit(1, 0.1f, 0, 0, 0.9f, 4));
	TestTrue(TEXT("Equal ranks choose earliest contact"), GASC_MeleeTrace::PreferHit(0, 0.1f, 4, 0, 0.9f, 0));
	TestTrue(TEXT("Exact ties use authored shape order"), GASC_MeleeTrace::PreferHit(0, 0.1f, 0, 0, 0.1f, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeInterpolationTest, "GASCourse.MeleeTrace.RotatingSocketInterpolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_MeleeInterpolationTest::RunTest(const FString& Parameters)
{
	const FTransform Previous(FQuat::Identity, FVector::ZeroVector);
	const FTransform Current(FRotator(0, 90, 0).Quaternion(), FVector::ZeroVector);
	const FVector EndLocal(100, 0, 0);
	const FTransform Middle = GASC_MeleeTrace::InterpolateSocket(Previous, Current, 0.5f);
	const FVector Tip = GASC_MeleeTrace::SampleSocketSegment(Middle, EndLocal, FVector::ZeroVector, 1.f);
	TestTrue(TEXT("Rotating tip follows radius, not the chord through the weapon"), FMath::IsNearlyEqual(Tip.Size(), 100.0, 0.001));
	TestTrue(TEXT("A dropped-frame midpoint lies on the 45 degree arc"), Tip.Equals(FVector(70.710678, 70.710678, 0), 0.001));
	const FVector Offset = GASC_MeleeTrace::SampleSocketSegment(Current, FVector::ZeroVector, FVector(10, 0, 0), 0);
	TestTrue(TEXT("Local offsets rotate with the socket"), Offset.Equals(FVector(0, 10, 0), 0.001));
	TestTrue(TEXT("Interpolation reaches the final pose"), GASC_MeleeTrace::InterpolateSocket(Previous, Current, 1.f).Equals(Current));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeTraceWorldTest, "GASCourse.MeleeTrace.WorldSweepsAndWindows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_MeleeTraceWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Subsystem = World->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
	if (!TestNotNull(TEXT("Melee subsystem"), Subsystem)) return false;
	Subsystem->DebugOptions.bRecord = true;
	Subsystem->DebugOptions.bEnabled = false;
	auto* Settings = GetMutableDefault<UGASC_MeleeSubsystem_Settings>();
	const auto SavedChannels = Settings->CollisionObjectTypes;
	const bool SavedObstacles = Settings->bCheckObstacles;
	Settings->CollisionObjectTypes = { UEngineTypes::ConvertToObjectType(ECC_Pawn) };
	Settings->bCheckObstacles = false;
	ON_SCOPE_EXIT { Settings->CollisionObjectTypes = SavedChannels; Settings->bCheckObstacles = SavedObstacles; };

	AActor* Attacker = World->SpawnActor<AActor>();
	auto* Source = NewObject<UStaticMeshComponent>(Attacker);
	Attacker->SetRootComponent(Source);
	// A transient mesh needs only sockets; no content assets or render data are required.
	auto* Mesh = NewObject<UStaticMesh>(Source);
	auto* StartSocket = NewObject<UStaticMeshSocket>(Mesh);
	StartSocket->SocketName = TEXT("Start");
	Mesh->AddSocket(StartSocket);
	auto* EndSocket = NewObject<UStaticMeshSocket>(Mesh);
	EndSocket->SocketName = TEXT("End");
	EndSocket->RelativeLocation = FVector(100, 0, 0);
	Mesh->AddSocket(EndSocket);
	Source->SetStaticMesh(Mesh);

	AActor* Target = World->SpawnActor<AActor>();
	auto* Body = NewObject<USphereComponent>(Target);
	Target->SetRootComponent(Body);
	Body->SetSphereRadius(5.f);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_Pawn);
	Body->SetCollisionResponseToAllChannels(ECR_Overlap);
	Body->RegisterComponentWithWorld(World);
	Body->SetWorldLocation(FVector(70.710678, 70.710678, 0));

	FGASC_MeleeTrace_Subsystem_Data Shape;
	Shape.TraceShape = UGASC_MeleeShape_Sphere::MakeSphereShape(5.f);
	Shape.SourceMeshComponent = Source;
	Shape.TraceSocket_Start = TEXT("Start");
	Shape.TraceSocket_End = TEXT("End");
	Shape.ShapeName = TEXT("Normal");
	Shape.Rank = 0; Shape.Group = 0;
	auto Sweetspot = Shape;
	Sweetspot.ShapeName = TEXT("Sweetspot");
	Sweetspot.Rank = 1;
	const FGuid Window = FGuid::NewGuid();
	Subsystem->RequestMeleeTraceWindow(Attacker, {Sweetspot, Shape}, Window, EGASC_MeleeHitPolicy::OncePerTarget);
	if (!TestEqual(TEXT("Both authored shapes register"), Subsystem->GetActiveTraces().Num(), 2)) return false;
	Source->SetWorldRotation(FRotator(0, 90, 0));
	Subsystem->ProcessMeleeTraces(0.1f);
	int32 Accepted = 0;
	for (const auto& Sample : Subsystem->GetDebugHistory())
		if (Sample.bAccepted) { ++Accepted; TestEqual(TEXT("Lowest rank wins independent of row order"), Sample.Rank, 0); }
	TestEqual(TEXT("Rotating shapes find the arc target and apply one hit"), Accepted, 1);
	TestTrue(TEXT("Previous pose advances to the current rotation"),
		Subsystem->GetActiveTraces()[0].PreviousSocketTransform.GetRotation().Equals(Source->GetComponentQuat()));

	Subsystem->ClearDebugHistory();
	Subsystem->ProcessMeleeTraces(0.1f);
	for (const auto& Sample : Subsystem->GetDebugHistory())
		TestFalse(TEXT("Stationary next update cannot repeat the old swing"), Sample.bAccepted);

	const FGuid OtherWindow = FGuid::NewGuid();
	Subsystem->RequestMeleeTraceWindow(Attacker, {Shape}, OtherWindow);
	TestTrue(TEXT("Cancel only the requested window"), Subsystem->CancelMeleeTrace(Window));
	TestFalse(TEXT("Closing window is no longer active"), Subsystem->IsMeleeTraceInProgress(Window));
	TestTrue(TEXT("Sibling notify survives"), Subsystem->IsMeleeTraceInProgress(OtherWindow));
	Subsystem->ProcessMeleeTraces(0.1f);
	TestEqual(TEXT("Only sibling window remains after final sweep"), Subsystem->GetActiveTraces().Num(), 1);
	Subsystem->ClearDebugHistory();
	Body->SetWorldLocation(FVector(-70.710678, 70.710678, 0));
	Source->SetWorldRotation(FRotator(0, 180, 0));
	Subsystem->CancelMeleeTrace(OtherWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	TestTrue(TEXT("Closing window sweeps its final motion before removal"),
		Subsystem->GetDebugHistory().ContainsByPredicate([](const auto& Sample) { return Sample.bAccepted; }));
	TestEqual(TEXT("Closing window releases all shapes"), Subsystem->GetActiveTraces().Num(), 0);

	// Queued notify end uses an event copy, unlike the asset event passed to begin.
	Attacker->AddInstanceComponent(Source);
	auto* EmittingMesh = NewObject<USkeletalMeshComponent>(Attacker);
	auto* Animation = NewObject<UAnimSequence>();
	Animation->SetSkeleton(NewObject<USkeleton>());
	Animation->GetController().InitializeModel();
	auto* Notify = NewObject<UGASC_MeleeTrace_NotifyState>(Animation);
	FGASC_MeleeTrace_TraceShapeData Row;
	Row.TraceShape = EGASC_MeleeTrace_TraceShape::Sphere;
	Row.TraceObject = EGASC_MeleeTrace_TraceObject::CharacterMesh;
	Row.MeshComponentNameOrTag = Source->GetFName();
	Row.StartSocket = TEXT("Start"); Row.EndSocket = TEXT("End");
	Notify->InlineShapes.Add(Row);
	FAnimNotifyEvent AssetEvent;
	AssetEvent.NotifyStateClass = Notify;
	FAnimNotifyEventReference First(&AssetEvent, Animation);
	First.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(1);
	FAnimNotifyEventReference Second(&AssetEvent, Animation);
	Second.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(2);
	Notify->NotifyBegin(EmittingMesh, Animation, 1.f, First);
	Notify->NotifyBegin(EmittingMesh, Animation, 1.f, Second);
	TestEqual(TEXT("Shared notify registers independent montage windows"), Subsystem->GetActiveTraces().Num(), 2);
	FAnimNotifyEvent CopiedEvent = AssetEvent;
	First.SetNotify(&CopiedEvent);
	Notify->NotifyEnd(EmittingMesh, Animation, First);
	Subsystem->ProcessMeleeTraces(0.1f);
	TestEqual(TEXT("Copied end event closes only its montage instance"), Subsystem->GetActiveTraces().Num(), 1);
	Second.SetNotify(&CopiedEvent);
	Notify->NotifyEnd(EmittingMesh, Animation, Second);
	Subsystem->ProcessMeleeTraces(0.1f);
	TestEqual(TEXT("No traces remain after both copied end events"), Subsystem->GetActiveTraces().Num(), 0);

	const int32 SavedDrawLimit = Settings->MaxDebugDrawSamples;
	ON_SCOPE_EXIT { Settings->MaxDebugDrawSamples = SavedDrawLimit; };
	Settings->MaxDebugDrawSamples = 4;
	TArray<FGASC_MeleeDebugSample> DenseSamples;
	DenseSamples.SetNum(100);
	for (int32 Index = 0; Index < DenseSamples.Num(); ++Index)
	{
		DenseSamples[Index].Actor = Attacker;
		DenseSamples[Index].ShapeIndex = Index;
	}
	Subsystem->UpdateLiveDebugSamples(DenseSamples);
	TestEqual(TEXT("Dense drawing uses a bounded buffer"), Subsystem->LiveDebugSamples.Num(), 4);
	TestEqual(TEXT("Draw sampling retains the end of the motion"), Subsystem->LiveDebugSamples.Last().ShapeIndex, 99);
	Subsystem->UpdateLiveDebugSamples(DenseSamples);
	TestEqual(TEXT("Successive frames cannot accumulate persistent geometry"), Subsystem->LiveDebugSamples.Num(), 4);
	Subsystem->DebugOptions.bEnabled = false;
	Subsystem->OnWorldPostActorTick(World, LEVELTICK_All, 0.1f);
	TestTrue(TEXT("Disabling drawing clears the live buffer"), Subsystem->LiveDebugSamples.IsEmpty());

	// Single socket ignores a populated/invalid end socket and uses the authored anchor.
	Row.SocketMode = EGASC_MeleeSocketMode::SingleSocket;
	Row.EndSocket = TEXT("MissingSocket");
	Row.ShapeAnchor = EGASC_MeleeShapeAnchor::Bottom;
	Row.SphereRadius = 10.f;
	auto AnchoredShape = Subsystem->CreateShapeDataFromRow(Row);
	const FGuid AnchorWindow = FGuid::NewGuid();
	Subsystem->ClearDebugHistory();
	Subsystem->RequestMeleeTraceWindow(Attacker, {AnchoredShape}, AnchorWindow);
	TestTrue(TEXT("Single socket does not require an end socket"), Subsystem->IsMeleeTraceInProgress(AnchorWindow));
	Subsystem->ProcessMeleeTraces(0.1f);
	TestEqual(TEXT("Single socket produces one spatial sample"), Subsystem->GetDebugHistory().Num(), 1);
	if (!Subsystem->GetDebugHistory().IsEmpty())
		TestTrue(TEXT("Bottom sphere anchor moves its center up by its radius"),
			Subsystem->GetDebugHistory()[0].End.GetLocation().Equals(Source->GetSocketLocation(TEXT("Start")) + FVector(0, 0, 10)));
	Subsystem->CancelMeleeTrace(AnchorWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	for (const auto Anchor : { EGASC_MeleeShapeAnchor::Center, EGASC_MeleeShapeAnchor::Bottom, EGASC_MeleeShapeAnchor::Top })
	{
		Row.SocketMode = EGASC_MeleeSocketMode::StartAndEnd;
		Row.EndSocket = TEXT("End");
		Row.ShapeAnchor = Anchor;
		Row.TraceShape = EGASC_MeleeTrace_TraceShape::Box;
		Row.BoxExtent = FVector(5, 10, 20);
		Row.bAlignToSocketSegment = false;
		Row.RotationOffset = FRotator(90, 0, 0);
		auto SegmentShape = Subsystem->CreateShapeDataFromRow(Row);
		const FGuid SegmentWindow = FGuid::NewGuid();
		Subsystem->ClearDebugHistory();
		Subsystem->RequestMeleeTraceWindow(Attacker, {SegmentShape}, SegmentWindow);
		Subsystem->ProcessMeleeTraces(0.1f);
		const auto& History = Subsystem->GetDebugHistory();
		if (TestTrue(TEXT("Two sockets retain spatial sampling"), History.Num() > 1))
		{
			const FVector Offset = (Source->GetComponentQuat() * Row.RotationOffset.Quaternion()).RotateVector(
				GASC_MeleeTrace::ShapeAnchorOffset(FCollisionShape::MakeBox(Row.BoxExtent), Anchor));
			TestTrue(TEXT("First segment shape uses its rotated anchor"), History[0].End.GetLocation().Equals(Source->GetSocketLocation(TEXT("Start")) + Offset));
			TestTrue(TEXT("Last segment shape uses its rotated anchor"), History.Last().End.GetLocation().Equals(Source->GetSocketLocation(TEXT("End")) + Offset));
		}
		Subsystem->CancelMeleeTrace(SegmentWindow);
		Subsystem->ProcessMeleeTraces(0.1f);
	}

	// Persona can seek without firing NotifyEnd. Validate the real preview playhead instead.
	// A second bone may guide orientation without becoming a sampled segment.
	auto AimRow = Row;
	AimRow.TraceShape = EGASC_MeleeTrace_TraceShape::Capsule;
	AimRow.CapsuleRadius = 5.f;
	AimRow.CapsuleHeight = 20.f;
	AimRow.SocketMode = EGASC_MeleeSocketMode::SingleSocket;
	AimRow.ShapeAnchor = EGASC_MeleeShapeAnchor::Bottom;
	AimRow.OrientationSocket = TEXT("End");
	AimRow.EndSocket = TEXT("IgnoredMissingSocket");
	AimRow.RotationOffset = FRotator::ZeroRotator;
	AimRow.bAlignToSocketSegment = false; // Explicit single-socket aiming is independent.
	auto AimShape = Subsystem->CreateShapeDataFromRow(AimRow);
	const FGuid AimWindow = FGuid::NewGuid();
	Subsystem->ClearDebugHistory();
	Subsystem->RequestMeleeTraceWindow(Attacker, {AimShape}, AimWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	if (TestEqual(TEXT("Orientation socket adds no spatial samples"), Subsystem->GetDebugHistory().Num(), 1))
	{
		const auto& Sample = Subsystem->GetDebugHistory()[0];
		const FVector Direction = (Source->GetSocketLocation(TEXT("End")) - Source->GetSocketLocation(TEXT("Start"))).GetSafeNormal();
		TestTrue(TEXT("Capsule Z aims from the anchor toward the orientation socket"), Sample.End.GetRotation().GetAxisZ().Equals(Direction));
		TestTrue(TEXT("Aimed capsule keeps its bottom at the start socket"), Sample.End.GetLocation().Equals(Source->GetSocketLocation(TEXT("Start")) + Direction * 20.f));
		TestEqual(TEXT("Aim distance does not resize the capsule"), Sample.Shape.GetCapsuleHalfHeight(), 20.f);
	}
	// Rotate the target direction by 180 degrees while the start socket stays still.
	const FVector SavedEndLocation = EndSocket->RelativeLocation;
	EndSocket->RelativeLocation = -SavedEndLocation;
	Subsystem->ClearDebugHistory();
	Subsystem->ProcessMeleeTraces(0.1f);
	const auto& AimHistory = Subsystem->GetDebugHistory();
	if (TestTrue(TEXT("Changing only the aim direction generates temporal substeps"), AimHistory.Num() > 1))
	{
		for (const auto& Sample : AimHistory)
			TestTrue(TEXT("Intermediate bottom anchors stay on the socket during an aim reversal"),
				(Sample.End.GetLocation() - Sample.End.GetRotation().GetAxisZ() * 20.f).Equals(Source->GetSocketLocation(TEXT("Start"))));
		const FVector Direction = (Source->GetSocketLocation(TEXT("End")) - Source->GetSocketLocation(TEXT("Start"))).GetSafeNormal();
		TestTrue(TEXT("Interpolation reaches the reversed aim direction"), AimHistory.Last().End.GetRotation().GetAxisZ().Equals(Direction));
	}
	Subsystem->CancelMeleeTrace(AimWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	EndSocket->RelativeLocation = SavedEndLocation;
	AimRow.RotationOffset = FRotator(15, 20, 30);
	AimShape = Subsystem->CreateShapeDataFromRow(AimRow);
	const FGuid OffsetAimWindow = FGuid::NewGuid();
	Subsystem->ClearDebugHistory();
	Subsystem->RequestMeleeTraceWindow(Attacker, {AimShape}, OffsetAimWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	if (TestEqual(TEXT("Offset aimed shape still has one spatial sample"), Subsystem->GetDebugHistory().Num(), 1))
	{
		const FQuat Expected = Source->GetSocketQuaternion(TEXT("Start")) * FRotationMatrix::MakeFromZ(SavedEndLocation).ToQuat() * AimRow.RotationOffset.Quaternion();
		TestTrue(TEXT("Authored rotation offset is applied after aiming"), Subsystem->GetDebugHistory()[0].End.GetRotation().Equals(Expected));
	}
	Subsystem->CancelMeleeTrace(OffsetAimWindow);
	Subsystem->ProcessMeleeTraces(0.1f);
	AimRow.OrientationSocket = TEXT("MissingAimSocket");
	const FGuid MissingAimWindow = FGuid::NewGuid();
	AddExpectedError(TEXT("Skipping invalid melee shape"), EAutomationExpectedErrorFlags::Contains, 1);
	Subsystem->RequestMeleeTraceWindow(Attacker, {Subsystem->CreateShapeDataFromRow(AimRow)}, MissingAimWindow);
	TestFalse(TEXT("Missing orientation socket rejects the trace"), Subsystem->IsMeleeTraceInProgress(MissingAimWindow));

	// Persona can seek without firing NotifyEnd. Validate the real preview playhead instead.
	World->WorldType = EWorldType::EditorPreview;
	ON_SCOPE_EXIT { World->WorldType = EWorldType::Game; };
	auto* Preview = NewObject<UAnimSingleNodeInstance>(EmittingMesh);
	EmittingMesh->AnimScriptInstance = Preview;
	auto* Montage = NewObject<UAnimMontage>();
	Montage->GetController().InitializeModel();
	Montage->SetCompositeLength(2.f);
	Preview->CurrentAsset = Montage;
	Subsystem->DebugOptions.bEnabled = true;
	Subsystem->DebugOptions.DrawDuration = 60.f;
	for (const float OutsideTime : { 1.f, 0.1f, 0.75f })
	{
		Preview->SetPosition(0.5f, false);
		const FGuid PreviewWindow = FGuid::NewGuid();
		Subsystem->RequestMeleeTraceWindow(Attacker, {AnchoredShape}, PreviewWindow);
		Subsystem->SetNotifyPreviewContext(PreviewWindow, EmittingMesh, Montage, 0.25f, 0.75f);
		Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
		TestTrue(TEXT("Preview draws inside the notify interval"), !Subsystem->LiveDebugSamples.IsEmpty());
		Preview->SetPosition(OutsideTime, false);
		Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
		TestFalse(TEXT("Seeking outside or to the end closes the preview window"), Subsystem->IsMeleeTraceInProgress(PreviewWindow));
		TestTrue(TEXT("Preview leaves no trail outside the notify despite a long draw duration"), Subsystem->LiveDebugSamples.IsEmpty());
	}
	// Sequence notify times are local to the segment, not to the montage timeline.
	auto& Slot = Montage->SlotAnimTracks.AddDefaulted_GetRef();
	auto& Segment = Slot.AnimTrack.AnimSegments.AddDefaulted_GetRef();
	Segment.SetAnimReference(Animation);
	Segment.StartPos = 1.f; Segment.AnimStartTime = 0.f; Segment.AnimEndTime = 1.f;
	Segment.AnimPlayRate = 1.f; Segment.LoopingCount = 1;
	Preview->SetPosition(1.5f, false);
	const FGuid SequenceWindow = FGuid::NewGuid();
	Subsystem->RequestMeleeTraceWindow(Attacker, {AnchoredShape}, SequenceWindow);
	Subsystem->SetNotifyPreviewContext(SequenceWindow, EmittingMesh, Animation, 0.25f, 0.75f);
	Subsystem->PrunePreviewWindows();
	TestTrue(TEXT("Sequence notify uses the montage segment's local time"), Subsystem->IsMeleeTraceInProgress(SequenceWindow));
	Preview->SetPosition(1.9f, false);
	Subsystem->PrunePreviewWindows();
	TestFalse(TEXT("Sequence notify expires beyond its local interval"), Subsystem->IsMeleeTraceInProgress(SequenceWindow));

	// Preview attachments use the selected character socket and feed weapon traces directly.
	auto* CharacterPreviewMesh = NewObject<USkeletalMesh>();
	CharacterPreviewMesh->SetSkeleton(Animation->GetSkeleton());
	auto* Grip = NewObject<USkeletalMeshSocket>(CharacterPreviewMesh);
	Grip->SocketName = TEXT("PreviewGrip");
	Grip->BoneName = TEXT("root");
	CharacterPreviewMesh->GetMeshOnlySocketList().Add(Grip);
	CharacterPreviewMesh->RebuildSocketMap();
	EmittingMesh->SetSkeletalMeshAsset(CharacterPreviewMesh);
	auto* SkeletonGrip = NewObject<USkeletalMeshSocket>(Animation->GetSkeleton());
	SkeletonGrip->SocketName = Grip->SocketName;
	SkeletonGrip->BoneName = Grip->BoneName;
	Animation->GetSkeleton()->Sockets.Add(SkeletonGrip);
	TestTrue(TEXT("Attachment picker includes the owning animation's skeleton sockets"), Notify->GetPreviewAttachmentPoints().Contains(TEXT("PreviewGrip")));
	Notify->PreviewMeshAsset = Mesh;
	Notify->PreviewAttachSocket = Grip->SocketName;
	Notify->PreviewRelativeTransform = FTransform(FVector(1, 2, 3));
	Notify->InlineShapes[0].TraceObject = EGASC_MeleeTrace_TraceObject::Weapon;
	Notify->InlineShapes[0].MeshComponentNameOrTag = NAME_None;
	Notify->NotifyBegin(EmittingMesh, Animation, 1.f, First);
	TWeakObjectPtr<UMeshComponent> PreviewWeapon;
	if (TestEqual(TEXT("Preview weapon supplies valid trace sockets"), Subsystem->GetActiveTraces().Num(), 1))
	{
		PreviewWeapon = Subsystem->GetActiveTraces()[0].SourceMeshComponent;
		TestTrue(TEXT("Static preview mesh component created"), PreviewWeapon->IsA<UStaticMeshComponent>());
		TestTrue(TEXT("Preview weapon attaches to the character mesh"), PreviewWeapon->GetAttachParent() == EmittingMesh);
		TestEqual(TEXT("Selected attachment socket is used"), PreviewWeapon->GetAttachSocketName(), Grip->SocketName);
		TestTrue(TEXT("Preview attachment offset is applied"), PreviewWeapon->GetRelativeTransform().Equals(Notify->PreviewRelativeTransform));
		TestTrue(TEXT("Preview weapon does not generate collision"), PreviewWeapon->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	Notify->NotifyEnd(EmittingMesh, Animation, First);
	TestTrue(TEXT("Ending preview notify releases its weapon component"), !PreviewWeapon.IsValid() || !PreviewWeapon->IsRegistered());
	TestTrue(TEXT("Ending preview notify releases weapon traces"), Subsystem->GetActiveTraces().IsEmpty());
	// Incomplete trace rows must not destroy the mesh designers need to inspect.
	FAnimNotifyEvent AuthoringEvent = AssetEvent;
	AuthoringEvent.SetTime(0.25f);
	AuthoringEvent.SetDuration(0.5f);
	FAnimNotifyEventReference AuthoringReference(&AuthoringEvent, Montage);
	Preview->SetPosition(0.5f, false);
	const auto ValidRow = Notify->InlineShapes[0];
	Notify->InlineShapes[0].StartSocket = TEXT("MissingWeaponSocket");
	AddExpectedError(TEXT("Skipping invalid melee shape"), EAutomationExpectedErrorFlags::Contains, 1);
	Notify->NotifyBegin(EmittingMesh, Animation, 0.5f, AuthoringReference);
	TestTrue(TEXT("Invalid sockets do not register collision traces"), Subsystem->GetActiveTraces().IsEmpty());
	if (TestEqual(TEXT("Invalid rows retain an attachment-only preview window"), Subsystem->Windows.Num(), 1))
	{
		PreviewWeapon = Subsystem->Windows.CreateConstIterator().Value().PreviewWeapon;
		TestTrue(TEXT("Preview remains registered with invalid trace sockets"), PreviewWeapon.IsValid() && PreviewWeapon->IsRegistered());
		// Exercise cleanup while another window could be processing collision requests.
		Subsystem->ProcessMeleeTraces(0.1f);
		Subsystem->PrunePreviewWindows();
		TestTrue(TEXT("Trace cleanup preserves an active authoring attachment"), PreviewWeapon.IsValid() && PreviewWeapon->IsRegistered());
	}
	Notify->NotifyEnd(EmittingMesh, Animation, AuthoringReference);
	TestTrue(TEXT("Notify end releases attachment-only previews"), !PreviewWeapon.IsValid() || !PreviewWeapon->IsRegistered());
	Notify->InlineShapes.Reset();
	Notify->NotifyBegin(EmittingMesh, Animation, 0.5f, AuthoringReference);
	if (TestEqual(TEXT("No trace rows are required to preview a weapon"), Subsystem->Windows.Num(), 1))
	{
		PreviewWeapon = Subsystem->Windows.CreateConstIterator().Value().PreviewWeapon;
		TestTrue(TEXT("Empty notify still attaches the weapon"), PreviewWeapon.IsValid() && PreviewWeapon->IsRegistered());
		Preview->SetPosition(0.75f, false);
		Subsystem->PrunePreviewWindows();
		TestTrue(TEXT("Seeking out releases an attachment with no traces"), !PreviewWeapon.IsValid() || !PreviewWeapon->IsRegistered());
	}
	Notify->InlineShapes.Add(ValidRow);
	// Reproduce Persona scrubbing into a notify without any NotifyBegin callback.
	Attacker->AddInstanceComponent(EmittingMesh);
	Montage->Notifies.Add(AuthoringEvent);
	Preview->SetPosition(0.5f, false);
	Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
	if (TestEqual(TEXT("Seeking into a notify creates its preview attachment"), Subsystem->Windows.Num(), 1))
	{
		PreviewWeapon = Subsystem->Windows.CreateConstIterator().Value().PreviewWeapon;
		TestTrue(TEXT("Seek-created attachment is registered on the preview character"),
			PreviewWeapon.IsValid() && PreviewWeapon->IsRegistered() && PreviewWeapon->GetAttachParent() == EmittingMesh);
		Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
		TestTrue(TEXT("Paused preview reuses its existing mesh"),
			Subsystem->Windows.Num() == 1 && Subsystem->Windows.CreateConstIterator().Value().PreviewWeapon == PreviewWeapon);
		if (PreviewWeapon.IsValid()) PreviewWeapon->DestroyComponent();
		Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
		if (TestEqual(TEXT("Persona cleanup restores exactly one attachment"), Subsystem->Windows.Num(), 1))
		{
			auto Restored = Subsystem->Windows.CreateConstIterator().Value().PreviewWeapon;
			TestTrue(TEXT("Removed attachment is replaced during the same notify"),
				Restored.IsValid() && Restored != PreviewWeapon && Restored->IsRegistered());
			PreviewWeapon = Restored;
		}
	}
	Preview->SetPosition(0.75f, false);
	Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
	TestTrue(TEXT("Seek to notify end removes the restored attachment"), !PreviewWeapon.IsValid() || !PreviewWeapon->IsRegistered());
	TestTrue(TEXT("No preview windows remain outside the interval"), Subsystem->Windows.IsEmpty());
	Preview->SetPosition(0.1f, false);
	Subsystem->OnWorldPostActorTick(World, LEVELTICK_ViewportsOnly, 0.1f);
	TestTrue(TEXT("Before notify start no attachment is created"), Subsystem->Windows.IsEmpty());
	Montage->Notifies.Reset();
	Notify->PreviewMeshAsset = CharacterPreviewMesh;
	UMeshComponent* SkeletalWeapon = Notify->CreatePreviewMesh(EmittingMesh);
	if (TestNotNull(TEXT("Skeletal preview assets supported"), SkeletalWeapon))
	{
		TestTrue(TEXT("Skeletal asset creates a skeletal component"), SkeletalWeapon->IsA<USkeletalMeshComponent>());
		const FGuid CleanupWindow = FGuid::NewGuid();
		Subsystem->RequestMeleeTraceWindow(Attacker, {AnchoredShape}, CleanupWindow);
		Subsystem->SetNotifyPreviewWeapon(CleanupWindow, SkeletalWeapon);
		Subsystem->SetNotifyPreviewContext(CleanupWindow, EmittingMesh, Montage, 0.25f, 0.75f);
		Subsystem->PrunePreviewWindows();
		TestFalse(TEXT("Seeking out releases the preview attachment"), SkeletalWeapon->IsRegistered());
	}
	for (const auto WorldType : { EWorldType::Game, EWorldType::PIE })
	{
		World->WorldType = WorldType;
		TestNull(TEXT("Preview assets never spawn in gameplay or PIE worlds"), Notify->CreatePreviewMesh(EmittingMesh));
		Montage->Notifies = {AuthoringEvent};
		Preview->SetPosition(0.5f, false);
		Subsystem->SynchronizePreviewAttachments();
		TestTrue(TEXT("Playhead synchronization never spawns attachments in Game or PIE"), Subsystem->Windows.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_MeleeAnchorTest, "GASCourse.MeleeTrace.ShapeAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_MeleeAnchorTest::RunTest(const FString& Parameters)
{
	const FCollisionShape Shapes[] = { FCollisionShape::MakeSphere(10.f), FCollisionShape::MakeCapsule(10.f, 30.f),
		FCollisionShape::MakeBox(FVector(5, 10, 20)) };
	const float Heights[] = { 10.f, 30.f, 20.f };
	const FQuat Rotation = FRotator(90, 0, 0).Quaternion();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("Center preserves existing placement"), GASC_MeleeTrace::ShapeAnchorOffset(Shapes[Index], EGASC_MeleeShapeAnchor::Center).IsZero());
		for (const auto Anchor : { EGASC_MeleeShapeAnchor::Bottom, EGASC_MeleeShapeAnchor::Top })
		{
			const float Sign = Anchor == EGASC_MeleeShapeAnchor::Bottom ? 1.f : -1.f;
			const FVector Offset = GASC_MeleeTrace::ShapeAnchorOffset(Shapes[Index], Anchor);
			TestTrue(TEXT("Anchor uses the authored half height"), Offset.Equals(FVector(0, 0, Heights[Index] * Sign)));
			for (const FVector Point : { FVector::ZeroVector, FVector(100, 0, 0) })
			{
				const FVector Center = Point + Rotation.RotateVector(Offset);
				TestTrue(TEXT("Rotated bottom/top remains on either segment endpoint"),
					(Center - Rotation.GetAxisZ() * Heights[Index] * Sign).Equals(Point));
			}
		}
	}
	return true;
}
#endif



