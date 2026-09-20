#include "Game/Systems/Debugging/GASC_RewindGhosts.h"
#if !UE_BUILD_SHIPPING
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformTime.h"
SIZE_T FGASC_RewindActorPose::Bytes() const
{
 SIZE_T N = sizeof(*this) + Name.GetAllocatedSize() + Meshes.GetAllocatedSize();
 for (const auto& M : Meshes) N += sizeof(*M) + M->LocalBones.GetAllocatedSize();
 return N;
}
FGASC_RewindActorPose GASC_CaptureRewindActor(AActor* Actor, bool& Truncated, double Deadline)
{
 FGASC_RewindActorPose Out;
 if (!IsValid(Actor)) return Out;
 Out.Actor = Actor; Out.Name = Actor->GetName(); Out.Transform = Actor->GetActorTransform();
 const FBox Box = Actor->CalculateComponentsBoundingBoxInLocalSpace();
 if (Box.IsValid) { Out.Center = Box.GetCenter(); Out.Extent = Box.GetExtent(); }
 TArray<AActor*> Actors = {Actor}; TSet<AActor*> Seen;
 for (int32 Index = 0; Index < Actors.Num(); ++Index)
 {
  AActor* Owner = Actors[Index]; if (!IsValid(Owner) || Seen.Contains(Owner)) continue;
  Seen.Add(Owner);
  if (FPlatformTime::Seconds() > Deadline) { Truncated = true; break; }
  TArray<AActor*> Attached; Owner->GetAttachedActors(Attached, true, false);
  for (AActor* Child : Attached) { if (Actors.Num() < 16) Actors.AddUnique(Child); else Truncated = true; }
  TInlineComponentArray<UMeshComponent*> Components(Owner);
  for (UMeshComponent* Component : Components)
  {
   if (!Component->IsVisible()) continue;
   if (Out.Meshes.Num() >= 8 || FPlatformTime::Seconds() > Deadline) { Truncated = true; break; }
   auto Pose = MakeShared<FGASC_RewindMeshPose>(); Pose->Transform = Component->GetComponentTransform();
   if (auto* Skel = Cast<USkeletalMeshComponent>(Component))
   {
    USkeletalMesh* Asset = Skel->GetSkeletalMeshAsset(); if (!Asset) continue;
    const auto& Ref = Asset->GetRefSkeleton();
    if (Ref.GetNum() > 512) { Truncated = true; continue; }
    Pose->SkeletalMesh.Reset(Asset);
    if (Skel->LeaderPoseComponent.IsValid())
    {
     // Modular bodies may have no local pose buffer; resolve the leader's historical pose.
     Pose->LocalBones.SetNum(Ref.GetNum());
     for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
     {
      const int32 Parent = Ref.GetParentIndex(Bone);
      Pose->LocalBones[Bone] = Skel->GetBoneTransform(Bone).GetRelativeTransform(Parent == INDEX_NONE ? Skel->GetComponentTransform() : Skel->GetBoneTransform(Parent));
     }
    }
    else
    {
     const auto Bones = Skel->GetBoneSpaceTransformsView();
     if (Bones.Num() != Ref.GetNum()) { Truncated = true; continue; }
     Pose->LocalBones.Append(Bones.GetData(), Bones.Num());
    }
   }
   else if (auto* Static = Cast<UStaticMeshComponent>(Component)) Pose->StaticMesh.Reset(Static->GetStaticMesh());
   if (Pose->SkeletalMesh.IsValid() || Pose->StaticMesh.IsValid()) Out.Meshes.Add(Pose);
  }
 }
 return Out;
}
FGASC_RewindGhostRenderer::FGASC_RewindGhostRenderer()
{
 // Project material ships with pre-authored skeletal/clothing/morph usage. Never mutate usage at runtime.
 auto* Base = LoadObject<UMaterial>(nullptr, TEXT("/Game/GASCourse/Game/Systems/Debugging/M_RewindGhost.M_RewindGhost"));
 if (Base && Base->GetUsageByFlag(MATUSAGE_SkeletalMesh) && Base->GetUsageByFlag(MATUSAGE_Clothing) && Base->GetUsageByFlag(MATUSAGE_MorphTargets) && Base->GetBlendMode() == BLEND_Opaque) GhostBaseMaterial.Reset(Base);
}
FGASC_RewindGhostRenderer::~FGASC_RewindGhostRenderer() { ClearGhostMeshes(); }
void FGASC_RewindGhostRenderer::ClearGhostMeshes()
{
 if (GhostActor.IsValid()) GhostActor->Destroy();
 GhostActor.Reset(); GhostMaterial.Reset(); GhostKey = 0; SkeletalPool.Empty(); StaticPool.Empty();
}
void FGASC_RewindGhostRenderer::Show(UWorld* World, uint64 Key, const TArray<TSharedPtr<FGASC_RewindMeshPose>>& Poses, int32 MaxMeshes)
{
 if (!World || Poses.IsEmpty()) { ClearGhostMeshes(); return; }
 if (GhostActor.IsValid() && GhostActor->GetWorld() != World) ClearGhostMeshes();
 if (GhostActor.IsValid() && GhostKey == Key && Key != 0) return;
 if (!GhostBaseMaterial.IsValid()) return;
 if (CreationFrame != GFrameCounter) { CreationFrame = GFrameCounter; CreatedThisFrame = 0; }
 MaxMeshes = FMath::Clamp(MaxMeshes, 1, 2064); SkippedMeshes = 0;
 if (!GhostActor.IsValid())
 {
  FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  GhostActor = World->SpawnActor<AActor>(Params); if (!GhostActor.IsValid()) return;
  GhostActor->SetActorEnableCollision(false); GhostActor->SetActorTickEnabled(false);
 }
 if (!GhostMaterial.IsValid())
 {
  GhostMaterial.Reset(UMaterialInstanceDynamic::Create(GhostBaseMaterial.Get(), GhostActor.Get()));
  GhostMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.38f, .23f, .025f, 1.f));
 }
 for (auto& M : SkeletalPool) if (M.IsValid()) M->SetVisibility(false);
 for (auto& M : StaticPool) if (M.IsValid()) M->SetVisibility(false);
 int32 SkelIndex = 0, StaticIndex = 0; bool Deferred = false;
 auto Init = [&](UMeshComponent* Mesh)
 {
  GhostActor->AddInstanceComponent(Mesh); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetGenerateOverlapEvents(false);
  Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false); Mesh->SetVisibility(false); Mesh->SetVisibleInRayTracing(false); Mesh->SetComponentTickEnabled(false); ++CreatedThisFrame;
 };
 auto Prepare = [&](UMeshComponent* Mesh)
 {
  for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I) if (Mesh->GetMaterial(I) != GhostMaterial.Get()) Mesh->SetMaterial(I, GhostMaterial.Get());
  if (!Mesh->IsRegistered()) Mesh->RegisterComponent();
  Mesh->SetComponentTickEnabled(false);
 };
 for (const auto& Pose : Poses)
 {
  if (SkelIndex + StaticIndex >= MaxMeshes) { ++SkippedMeshes; continue; }
  UMeshComponent* Mesh = nullptr;
  if (Pose->SkeletalMesh.IsValid() && !Pose->LocalBones.IsEmpty())
  {
   if (SkelIndex == SkeletalPool.Num())
   {
    if (CreatedThisFrame >= 2) { Deferred = true; ++SkippedMeshes; continue; }
    if (SkeletalPool.Num() + StaticPool.Num() >= MaxMeshes && StaticIndex < StaticPool.Num()) { if (StaticPool.Last().IsValid()) StaticPool.Last()->DestroyComponent(); StaticPool.Pop(); }
    if (SkeletalPool.Num() + StaticPool.Num() >= MaxMeshes) { ++SkippedMeshes; continue; }
    auto* NewMesh = NewObject<UPoseableMeshComponent>(GhostActor.Get(), NAME_None, RF_Transient); Init(NewMesh); SkeletalPool.Add(NewMesh);
   }
   auto* Skel = SkeletalPool[SkelIndex++].Get(); Mesh = Skel;
   if (!Skel) { ClearGhostMeshes(); return; }
   if (Skel->GetSkinnedAsset() != Pose->SkeletalMesh.Get()) { if (Skel->IsRegistered()) Skel->UnregisterComponent(); Skel->SetSkinnedAssetAndUpdate(Pose->SkeletalMesh.Get()); }
   Skel->SetWorldTransform(Pose->Transform); Prepare(Skel);
   if (Skel->BoneSpaceTransforms.Num() == Pose->LocalBones.Num()) { Skel->BoneSpaceTransforms = Pose->LocalBones; Skel->MarkRefreshTransformDirty(); Skel->RefreshBoneTransforms(); }
  }
  else if (Pose->StaticMesh.IsValid())
  {
   if (StaticIndex == StaticPool.Num())
   {
    if (CreatedThisFrame >= 2) { Deferred = true; ++SkippedMeshes; continue; }
    if (SkeletalPool.Num() + StaticPool.Num() >= MaxMeshes && SkelIndex < SkeletalPool.Num()) { if (SkeletalPool.Last().IsValid()) SkeletalPool.Last()->DestroyComponent(); SkeletalPool.Pop(); }
    if (SkeletalPool.Num() + StaticPool.Num() >= MaxMeshes) { ++SkippedMeshes; continue; }
    auto* NewMesh = NewObject<UStaticMeshComponent>(GhostActor.Get(), NAME_None, RF_Transient); Init(NewMesh); StaticPool.Add(NewMesh);
   }
   auto* Static = StaticPool[StaticIndex++].Get(); Mesh = Static;
   if (!Static) { ClearGhostMeshes(); return; }
   if (Static->GetStaticMesh() != Pose->StaticMesh.Get()) Static->SetStaticMesh(Pose->StaticMesh.Get());
   Static->SetWorldTransform(Pose->Transform);
  }
  if (Mesh) { Prepare(Mesh); Mesh->SetVisibility(true); }
 }
 GhostKey = Deferred ? 0 : Key;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "PreviewScene.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_PackagedRewindRenderTest, "GASCourse.Debug.RewindPackagedMeshes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FGASC_PackagedRewindRenderTest::RunTest(const FString& Parameters)
{
 if (!FApp::CanEverRender()) { AddError(TEXT("This regression requires a rendering RHI.")); return false; }
 auto Preview = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues().SetEditor(false).SetCreatePhysicsScene(false));
 UWorld* World = Preview->GetWorld();
 auto* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DummyTraining/Meshes/SK_Dummy_Training.SK_Dummy_Training"));
 auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (!TestNotNull(TEXT("Cooked skeletal mesh"), Body) || !TestNotNull(TEXT("Cooked static mesh"), Cube)) return false;
 auto RendererOwner = MakeShared<FGASC_RewindGhostRenderer>();
 auto& Renderer = *RendererOwner;
 if (!TestTrue(TEXT("Cooked project material with all required usage flags"), Renderer.GhostBaseMaterial.IsValid())) return false;
#if WITH_EDITOR
 FAssetCompilingManager::Get().FinishAllCompilation();
 if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
#endif
 auto A = MakeShared<FGASC_RewindMeshPose>(); A->SkeletalMesh.Reset(Body); A->Transform = FTransform::Identity; A->LocalBones = Body->GetRefSkeleton().GetRefBonePose();
 auto B = MakeShared<FGASC_RewindMeshPose>(); B->StaticMesh.Reset(Cube); B->Transform = FTransform(FQuat::Identity, FVector(0, 0, 90), FVector(.5));
 Renderer.Show(World, 1, {A, B}, 8);
 TestEqual(TEXT("Skeletal ghost instantiated"), Renderer.SkeletalPool.Num(), 1); TestEqual(TEXT("Weapon proxy instantiated"), Renderer.StaticPool.Num(), 1);
 auto* CaptureActor = World->SpawnActor<AActor>();
 auto* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor); CaptureActor->AddInstanceComponent(Capture);
 auto* Texture = NewObject<UTextureRenderTarget2D>(Capture); Texture->ClearColor = FLinearColor::Blue; Texture->InitAutoFormat(64, 64); Texture->UpdateResourceImmediate(true);
 Capture->TextureTarget = Texture; Capture->CaptureSource = SCS_SceneColorHDR; Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
 Capture->bAlwaysPersistRenderingState = true;
 UKismetRenderingLibrary::ClearRenderTarget2D(World, Texture, FLinearColor::Blue);
 TestTrue(TEXT("Render-target readback sees the clear color"), UKismetRenderingLibrary::ReadRenderTargetRawPixel(World, Texture, 32, 32, false).B > .5f);
 Capture->SetWorldLocationAndRotation(FVector(300, 0, 90), FRotator(0, 180, 0)); Capture->RegisterComponent();
 // Allow normal frame preparation and asynchronous mesh/material work before readback.
 ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Preview, RendererOwner, Capture, Texture]()
 {
 UWorld* World = Preview->GetWorld(); auto& Renderer = *RendererOwner;
 Capture->CaptureScene();
 const FLinearColor Pixel = UKismetRenderingLibrary::ReadRenderTargetRawPixel(World, Texture, 32, 32, false);
 const auto Pixels = UKismetRenderingLibrary::ReadRenderTargetRawPixelArea(World, Texture, 0, 0, 63, 63, false);
 int32 GoldPixels = 0;
 for (const auto& P : Pixels) { if (P.R > .01f && P.R > P.B && P.G > P.B) ++GoldPixels; }
 TestTrue(TEXT("Gold mesh pixels are visible"), GoldPixels > 0);
 TestTrue(TEXT("Opaque gold geometry reaches the render target"), Pixel.R > .01f && Pixel.R > Pixel.B && Pixel.G > Pixel.B);
 AddInfo(FString::Printf(TEXT("Rewind mesh pixel: %s; material %s"), *Pixel.ToString(), *Renderer.GhostBaseMaterial->GetPathName()));
 Renderer.StaticPool[0]->SetVisibility(false);
 Capture->CaptureScene();
 const auto BodyPixels = UKismetRenderingLibrary::ReadRenderTargetRawPixelArea(World, Texture, 0, 0, 63, 63, false);
 TestTrue(TEXT("Skeletal body renders gold without the weapon proxy"), BodyPixels.ContainsByPredicate([](const FLinearColor& P) { return P.R > .01f && P.R > P.B && P.G > P.B; }));
 Renderer.ClearGhostMeshes(); Capture->CaptureScene(); UKismetRenderingLibrary::ReadRenderTargetRawPixel(World, Texture, 32, 32);
 }, 1.f));
 return true;
}
#endif
#endif


