#include "Game/Systems/Debugging/GASC_RewindDebugControls.h"
#if !UE_BUILD_SHIPPING
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "imgui.h"
namespace
{
 struct FFreeCamera
 {
  TWeakObjectPtr<APawn> Original, Camera;
  FRotator OriginalRotation;
  bool TickPaused = false, CameraMovable = false;
  uint64 UpdatedFrame = MAX_uint64;
 };
 TMap<TWeakObjectPtr<APlayerController>, FFreeCamera> Cameras;
}
namespace
{
 void ToggleFreeCamera(UWorld* World, APlayerController* PC, FString& Error)
 {
  if (!World || !PC || !PC->IsLocalController() || !PC->HasAuthority()) return;
  FFreeCamera* State = Cameras.Find(PC);
  if (State)
  {
   if (State->Original.IsValid() && (!State->Original->GetController() || State->Original->GetController() == PC))
   {
    PC->Possess(State->Original.Get());
    if (PC->GetPawn() == State->Original.Get())
    {
     PC->SetControlRotation(State->OriginalRotation); World->bIsCameraMoveableWhenPaused = State->CameraMovable; PC->SetTickableWhenPaused(State->TickPaused);
     if (State->Camera.IsValid()) State->Camera->Destroy(); Cameras.Remove(PC); Error.Reset();
    }
    else Error = TEXT("Could not repossess the original character.");
   }
   else Error = TEXT("Original character was destroyed or is controlled by someone else.");
  }
  else if (PC->GetPawn())
  {
   FVector Location; FRotator Rotation; PC->GetPlayerViewPoint(Location, Rotation);
   FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   auto* Camera = World->SpawnActor<ASpectatorPawn>(Location, Rotation, Params);
   if (Camera)
   {
    FFreeCamera NewState; NewState.Original = PC->GetPawn(); NewState.OriginalRotation = PC->GetControlRotation(); NewState.Camera = Camera;
    NewState.TickPaused = PC->PrimaryActorTick.bTickEvenWhenPaused; NewState.CameraMovable = World->bIsCameraMoveableWhenPaused;
    Camera->bAddDefaultMovementBindings = false; Camera->SetActorEnableCollision(false); Camera->SetActorTickEnabled(false);
    if (Camera->GetMovementComponent()) Camera->GetMovementComponent()->SetComponentTickEnabled(false);
    PC->Possess(Camera);
    if (PC->GetPawn() == Camera) { PC->SetControlRotation(Rotation); World->bIsCameraMoveableWhenPaused = true; PC->SetTickableWhenPaused(true); Cameras.Add(PC, NewState); Error.Reset(); }
    else { Camera->Destroy(); Error = TEXT("Controller refused free-camera possession."); }
   }
   else Error = TEXT("Could not spawn the free camera.");
  }
  else Error = TEXT("No possessed character is available to unpossess.");
 }
}
bool GASC_BeginRewindSettings()
{
 if (ImGui::Button("Settings")) ImGui::OpenPopup("RewindSettings");
 return ImGui::BeginPopup("RewindSettings");
}
void GASC_DrawRewindWorldControls(UWorld* World, FString& Error)
{
 APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
 for (auto It = Cameras.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 const bool Paused = World && World->IsPaused();
 ImGui::BeginDisabled(!World || Paused);
 if (ImGui::Button("Pause game")) { if (!UGameplayStatics::SetGamePaused(World, true)) Error = TEXT("Game mode refused pause."); else Error.Reset(); }
 ImGui::EndDisabled(); ImGui::SameLine(); ImGui::BeginDisabled(!World || !Paused);
 if (ImGui::Button("Resume game")) { if (!UGameplayStatics::SetGamePaused(World, false)) Error = TEXT("Game mode refused resume."); else Error.Reset(); }
 ImGui::EndDisabled(); ImGui::SameLine();
 FFreeCamera* State = Cameras.Find(PC);
 ImGui::BeginDisabled(!PC || !PC->IsLocalController() || !PC->HasAuthority());
 if (ImGui::Button(State ? "Return to character" : "Unpossess / Free camera"))
 {
  ToggleFreeCamera(World, PC, Error);
 }
 ImGui::EndDisabled();
 if (PC && !PC->HasAuthority()) ImGui::TextDisabled("Free camera requires local authority (standalone/listen host).");
 if (auto* Current = Cameras.Find(PC); Current && Current->Camera.IsValid())
 {
  ImGui::TextUnformatted("Free camera: hold RMB + WASD, Q/E vertical, Shift faster. Works while paused.");
  if (Current->UpdatedFrame != GFrameCounter && ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::GetIO().WantTextInput)
  {
   Current->UpdatedFrame = GFrameCounter;
   const auto& IO = ImGui::GetIO(); FRotator Rotation = PC->GetControlRotation(); Rotation.Yaw += IO.MouseDelta.x * .15f; Rotation.Pitch = FMath::ClampAngle(Rotation.Pitch - IO.MouseDelta.y * .15f, -89.f, 89.f); Rotation.Roll = 0;
   FVector Direction = Rotation.Vector() * (float(ImGui::IsKeyDown(ImGuiKey_W)) - float(ImGui::IsKeyDown(ImGuiKey_S)));
   Direction += FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y) * (float(ImGui::IsKeyDown(ImGuiKey_D)) - float(ImGui::IsKeyDown(ImGuiKey_A)));
   Direction.Z += float(ImGui::IsKeyDown(ImGuiKey_E)) - float(ImGui::IsKeyDown(ImGuiKey_Q));
   Current->Camera->SetActorLocation(Current->Camera->GetActorLocation() + Direction.GetClampedToMaxSize(1.f) * FMath::Min(IO.DeltaTime, .1f) * (IO.KeyShift ? 2400.f : 800.f));
   PC->SetControlRotation(Rotation);
   if (PC->PlayerCameraManager) PC->PlayerCameraManager->UpdateCamera(IO.DeltaTime);
  }
 }
 if (!Error.IsEmpty()) ImGui::TextWrapped("%s", TCHAR_TO_UTF8(*Error));
}
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_RewindFreeCameraTest, "GASCourse.Debug.RewindFreeCamera", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGASC_RewindFreeCameraTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
 auto* PC = World->SpawnActor<APlayerController>();
 PC->Player = NewObject<ULocalPlayer>(GEngine);
 auto* Original = World->SpawnActor<APawn>();
 PC->Possess(Original); PC->SetControlRotation(FRotator(15, 35, 0));
 const bool WasTickPaused = PC->PrimaryActorTick.bTickEvenWhenPaused;
 const bool WasCameraMovable = World->bIsCameraMoveableWhenPaused;
 FString Error; ToggleFreeCamera(World, PC, Error);
 TestTrue(TEXT("Free camera unpossesses original"), Original->GetController() == nullptr);
 TestTrue(TEXT("Controller owns spectator"), PC->GetPawn() && PC->GetPawn()->IsA<ASpectatorPawn>());
 TestTrue(TEXT("Paused camera movement enabled"), World->bIsCameraMoveableWhenPaused && PC->PrimaryActorTick.bTickEvenWhenPaused);
 TWeakObjectPtr<APawn> Camera = PC->GetPawn();
 ToggleFreeCamera(World, PC, Error);
 TestTrue(TEXT("Return restores original possession"), PC->GetPawn() == Original);
 TestTrue(TEXT("Return restores orientation"), PC->GetControlRotation().Equals(FRotator(15, 35, 0)));
 TestTrue(TEXT("Return restores pause settings"), World->bIsCameraMoveableWhenPaused == WasCameraMovable && PC->PrimaryActorTick.bTickEvenWhenPaused == WasTickPaused);
 TestTrue(TEXT("Transient camera removed"), !Camera.IsValid());
 TestTrue(TEXT("Shared camera state removed"), !Cameras.Contains(PC));
 TestTrue(TEXT("No control error"), Error.IsEmpty());
 return true;
}
#endif
#endif

