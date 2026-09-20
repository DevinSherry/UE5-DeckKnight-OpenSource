#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortDistance.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortInputAngle.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortCameraAngle.h"
#include "Game/Systems/Targeting/Filtering/GASCourse_FilterTargetByInput.h"
#include "Game/Systems/Targeting/Filtering/GASCourse_TargetFilterOnScreen.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_TargetFilter_ActorClass.h"
#include "Game/Character/Components/Targeting/GASC_PlayerTargetingComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace GASCTargetingDebug
{
	static APawn* Pawn(FTargetingRequestHandle H) { const auto* S = FTargetingSourceContext::Find(H); return S ? Cast<APawn>(S->SourceActor) : nullptr; }
	static bool Disabled(const TCHAR* Name) { const auto* C = IConsoleManager::Get().FindConsoleVariable(Name); return C && C->GetBool(); }
	static FTargetingDebugPrimitive Arrow(FVector Start, FVector End) { FTargetingDebugPrimitive P; P.Transform.SetLocation(Start); P.Vector = End; return P; }
	static TArray<FTargetingDebugPrimitive> InputArrow(FTargetingRequestHandle H, FVector Input)
	{
		if (const APawn* P = Pawn(H))
		{
			const FVector Origin = P->GetActorLocation();
			auto InputVisual = Arrow(Origin, Origin + Input.GetSafeNormal() * 600); InputVisual.Color = FLinearColor(1.f, .65f, .02f);
			TArray<FTargetingDebugPrimitive> Out = { InputVisual };
			const FTransform Transform = P->GetActorTransform();
			for (int32 I = 0; I < 3; ++I)
			{
				auto Axis = Arrow(Origin, Origin + Transform.GetUnitAxis(I == 0 ? EAxis::X : I == 1 ? EAxis::Y : EAxis::Z) * 150);
				Axis.Color = I == 0 ? FLinearColor::Red : I == 1 ? FLinearColor::Green : FLinearColor::Blue;
				Out.Add(Axis);
			}
			return Out;
		}
		return {};
	}
}

#endif
FString UGASCourse_TargetSortDistance::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	if (GASCTargetingDebug::Disabled(TEXT("GASCourseDebug.Targeting.Disable.Sort.Distance"))) return TEXT("Distance scoring disabled by CVar; added zero.");
	const AActor* A = Target.HitResult.GetActor();
	return SourcePawn && A ? FString::Printf(TEXT("Distance %.2f; max %.2f; multiplier %.3f; %s"), FVector::Distance(SourcePawn->GetActorLocation(), A->GetActorLocation()), MaxDistance, FinalScoreMultiplier, ScoreCurve ? TEXT("custom curve") : TEXT("linear falloff")) : TEXT("Missing source pawn or target actor; added zero.");

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASCourse_TargetSortDistance::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const
{
#if !UE_BUILD_SHIPPING

	if (!SourcePawn) return {};
	FTargetingDebugPrimitive P; P.Shape = ETargetingDebugShape::Circle; P.Transform.SetLocation(SourcePawn->GetActorLocation()); P.Radius = MaxDistance; return { P };

#else
 return {};
#endif
}
FString UGASCourse_TargetSortInputAngle::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	if (GASCTargetingDebug::Disabled(TEXT("GASCourseDebug.Targeting.Disable.Sort.InputAngle"))) return TEXT("Input scoring disabled by CVar; added zero.");
	const AActor* A = Target.HitResult.GetActor();
	if (!SourcePawn || !SourcePlayerController || !A) return TEXT("Missing source pawn, player controller or target; added zero.");
	const float Dot = FVector::DotProduct(InputDirection.GetSafeNormal(), (A->GetActorLocation() - SourcePawn->GetActorLocation()).GetSafeNormal());
	return FString::Printf(TEXT("Input %s; dot %.4f; multiplier %.3f%s"), *InputDirection.ToCompactString(), Dot, FinalScoreMultiplier, InputDirection.IsNearlyZero() ? TEXT("; zero input") : TEXT(""));

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASCourse_TargetSortInputAngle::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const
{
#if !UE_BUILD_SHIPPING

	return GASCTargetingDebug::InputArrow(Handle, InputDirection);

#else
 return {};
#endif
}
FString UGASCourse_TargetSortCameraAngle::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	if (GASCTargetingDebug::Disabled(TEXT("GASCourseDebug.Targeting.Disable.Sort.CameraAngle"))) return TEXT("Camera scoring disabled by CVar; added zero.");
	const AActor* A = Target.HitResult.GetActor();
	if (!SourcePawn || !CameraManager || !A) return TEXT("Missing source, camera or target; added zero.");
	const float Dot = FVector::DotProduct(CameraManager->GetActorForwardVector(), (A->GetActorLocation() - SourcePawn->GetActorLocation()).GetSafeNormal());
	return FString::Printf(TEXT("Angle %.2f degrees; max %.2f; multiplier %.3f"), FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f))), MaxCameraAngle, FinalScoreMultiplier);

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASCourse_TargetSortCameraAngle::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const
{
#if !UE_BUILD_SHIPPING

	TArray<FTargetingDebugPrimitive> Out;
	if (!SourcePawn || !CameraManager) return Out;
	const FVector Start = SourcePawn->GetActorLocation();
	Out.Add(GASCTargetingDebug::Arrow(Start, Start + CameraManager->GetActorForwardVector() * 500));
	if (const auto* Results = FTargetingDefaultResultsSet::Find(Handle)) for (const auto& T : Results->TargetResults)
	{
		if (const AActor* A = T.HitResult.GetActor()) { auto P = GASCTargetingDebug::Arrow(Start, A->GetActorLocation()); P.Shape = ETargetingDebugShape::Line; P.Color = FLinearColor::Blue; Out.Add(P); }
	}
	return Out;

#else
 return {};
#endif
}
FString UGASCourse_FilterTargetByInput::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	if (GASCTargetingDebug::Disabled(TEXT("GASCourseDebug.Targeting.Disable.Filter.InputAngle"))) return TEXT("Input filter disabled by CVar.");
	const APawn* P = GASCTargetingDebug::Pawn(Handle);
	const APlayerController* PC = P ? Cast<APlayerController>(P->GetController()) : nullptr;
	if (!P || !PC || !PC->GetPawn() || !Target.HitResult.HasValidHitObjectHandle()) return TEXT("Missing valid hit/source/player controller; filter bypassed.");
	const FVector Input = P->GetLastMovementInputVector(); const FVector Delta = Target.HitResult.Location - PC->GetPawn()->GetActorLocation();
	const FVector2D Input2D(Input.X, Input.Y), Target2D(Delta.X, Delta.Y);
	const float Dot = FVector2D::DotProduct(Input2D.GetSafeNormal(), Target2D.GetSafeNormal());
	if (Input2D.Size() < MinInputThreshold)
	{
		if (bFiltered) return FString::Printf(TEXT("No input: magnitude %.3f < %.3f; FilterAllTargetsWhenNoInput enabled."), Input2D.Size(), MinInputThreshold);
		return !bFilterAllTargetsWhenNoInput ? TEXT("Input below threshold; no-input policy retains targets.") : TEXT("Input below threshold; recent-melee-target exception retained this target.");
	}
	return FString::Printf(TEXT("%s: input dot %.4f %s minimum %.4f"), bFiltered ? TEXT("Rejected") : TEXT("Retained"), Dot, bFiltered ? TEXT("<") : TEXT(">="), MinInputDotAngle);

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASCourse_FilterTargetByInput::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const
{
#if !UE_BUILD_SHIPPING

	const APawn* P = GASCTargetingDebug::Pawn(Handle); if (!P) return {};
	FVector Input = P->GetLastMovementInputVector(); Input.Z = 0; return GASCTargetingDebug::InputArrow(Handle, Input);

#else
 return {};
#endif
}
FString UGASCourse_TargetFilterOnScreen::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	const APawn* P = GASCTargetingDebug::Pawn(Handle); const APlayerController* PC = P ? Cast<APlayerController>(P->GetController()) : nullptr;
	if (!PC || !Target.HitResult.HasValidHitObjectHandle()) return TEXT("Missing source/controller/hit; filter bypassed.");
	int32 X, Y; PC->GetViewportSize(X, Y); FVector2D Screen = FVector2D::ZeroVector;
	const bool Projected = PC->ProjectWorldLocationToScreen(Target.HitResult.Location, Screen, false);
	return FString::Printf(TEXT("%s: screen (%.1f, %.1f), allowed X [%.1f,%d), Y [%.1f,%d); projection %s"), bFiltered ? TEXT("Outside screen bounds") : TEXT("Inside screen bounds"), Screen.X, Screen.Y, ViewThresholdMinX, X, ViewThresholdMinY, Y, Projected ? TEXT("succeeded") : TEXT("failed (predicate does not check this)"));

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASCourse_TargetFilterOnScreen::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const
{
#if !UE_BUILD_SHIPPING

	TArray<FTargetingDebugPrimitive> Out;
	const APawn* P = GASCTargetingDebug::Pawn(Handle); const APlayerController* PC = P ? Cast<APlayerController>(P->GetController()) : nullptr; if (!PC) return Out;
	int32 X, Y; PC->GetViewportSize(X, Y);
	const FVector2D Screen[] = {{ViewThresholdMinX, ViewThresholdMinY}, {double(X), ViewThresholdMinY}, {double(X), double(Y)}, {ViewThresholdMinX, double(Y)}};
	FVector Corners[4], Origins[4];
	for (int32 I = 0; I < 4; ++I) { FVector Direction; if (!PC->DeprojectScreenPositionToWorld(Screen[I].X, Screen[I].Y, Origins[I], Direction)) return {}; Corners[I] = Origins[I] + Direction * 1500; }
	for (int32 I = 0; I < 4; ++I) { auto Edge = GASCTargetingDebug::Arrow(Corners[I], Corners[(I+1)%4]); Edge.Shape = ETargetingDebugShape::Line; Out.Add(Edge); Edge.Transform.SetLocation(Origins[I]); Edge.Vector = Corners[I]; Out.Add(Edge); }
	return Out;

#else
 return {};
#endif
}
FString UGASC_TargetFilter_ActorClass::GetEvaluationDebugTargetInfo_Implementation(const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool bFiltered) const
{
#if !UE_BUILD_SHIPPING

	const AActor* A = Target.HitResult.GetActor(); if (!A) return TEXT("No valid actor.");
	for (UClass* C : IgnoredActorClassFilters) if (C && A->IsA(C)) return TEXT("Matches ignored class: ") + C->GetName();
	for (UClass* C : RequiredActorClassFilters) if (C && A->IsA(C)) return TEXT("Matches required class: ") + C->GetName();
	return RequiredActorClassFilters.IsEmpty() ? TEXT("No required class restriction.") : TEXT("Does not match any required actor class.");

#else
 return {};
#endif
}
TArray<FTargetingDebugPrimitive> UGASC_TargetFilter_ActorClass::GetEvaluationDebugPrimitives_Implementation(const FTargetingRequestHandle& Handle) const {
#if !UE_BUILD_SHIPPING
 return {}; 
#else
 return {};
#endif
}
