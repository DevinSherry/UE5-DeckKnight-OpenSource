// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Systems/Targeting/Filtering/GASCourse_FilterTargetByInput.h"
#include "DrawDebugHelpers.h"
#include "Game/Character/Components/Targeting/GASC_PlayerTargetingComponent.h"

namespace GASCourse_TargetingSystemCVars
{
	static bool bEnableInputFilteringDebug = false;
	FAutoConsoleVariableRef CvarEnableInputFilterDebugging(
		TEXT("GASCourseDebug.Targeting.EnableDebug.Filter.InputAngle"),
		bEnableInputFilteringDebug,
		TEXT("Enable on-screen debugging for input based target filtering (Enabled: true, Disabled: false)")
	);

	static bool bDisableInputTargetFiltering = false;
	FAutoConsoleVariableRef CvarDisableInputTargetFiltering(
		TEXT("GASCourseDebug.Targeting.Disable.Filter.InputAngle"),
		bDisableInputTargetFiltering,
		TEXT("Disable input based filtering for targeting (Enabled: true, Disabled: false)")
	);
}

bool UGASCourse_FilterTargetByInput::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	// Debug CVar to globally disable this filter
	if (GASCourse_TargetingSystemCVars::bDisableInputTargetFiltering)
	{
		return false;
	}

	// If no valid hit, defer to parent behavior
	if (!TargetData.HitResult.HasValidHitObjectHandle())
	{
		return Super::ShouldFilterTarget(TargetingHandle, TargetData);
	}

	bool bShouldFilter = false;

	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return false;
	}

	APawn* InstigatorPawn = Cast<APawn>(SourceContext->SourceActor);
	if (!InstigatorPawn)
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PlayerController || !PlayerController->GetPawn())
	{
		return false;
	}

	AActor* TargetActor = TargetData.HitResult.GetActor();
	const FVector TargetLocation = TargetData.HitResult.Location;
	
	// Target direction (2D)
	const FVector PawnLoc = PlayerController->GetPawn()->GetActorLocation();
	const FVector ToTarget3D = TargetLocation - PawnLoc;

	FVector2D TargetDir2D(ToTarget3D.X, ToTarget3D.Y);
	TargetDir2D.Normalize();

	// Input direction (RAW magnitude check first!)
	const FVector RawInput3D = InstigatorPawn->GetLastMovementInputVector();
	FVector2D InputDir2D(RawInput3D.X, RawInput3D.Y);

	if (InputDir2D.Size() < MinInputThreshold)
	{
		if (bFilterAllTargetsWhenNoInput)
		{
			bShouldFilter = true;
		}
		else
		{
			return false;
		}
	}

	FVector2D InputDir2DFlattened = InputDir2D.GetSafeNormal();

	// Dot in 2D (top-down)
	const float DotAngle = FVector2D::DotProduct(TargetDir2D, InputDir2DFlattened);

	// Filter out targets not aligned enough with intent
	if (!bShouldFilter && DotAngle < MinInputDotAngle)
	{
		bShouldFilter = true;
	}

	// Preserve recent melee target even if it fails the input filter
	if (UGASC_PlayerTargetingComponent* PlayerTargetingComponent =
		PlayerController->FindComponentByClass<UGASC_PlayerTargetingComponent>())
	{
		if (PreserveRecentMeleeTarget && TargetActor &&
			PlayerTargetingComponent->IsActorRecentMeleeTarget(TargetActor))
		{
			InputDir2D = FVector2D(RawInput3D.X, RawInput3D.Y);
			UE_LOGFMT(LogTemp, Warning, "InputDirection: {0}", InputDir2D.ToString());
			if (InputDir2D.Size() < MinInputThreshold)
			{
				bShouldFilter = false;
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (GASCourse_TargetingSystemCVars::bEnableInputFilteringDebug)
	{
		if (UWorld* World = InstigatorPawn->GetWorld())
		{
			if (TargetActor)
			{
				FVector BoundsOrigin, BoundsExtent;
				TargetActor->GetActorBounds(true, BoundsOrigin, BoundsExtent, false);

				const FColor DebugColor = bShouldFilter ? FColor::Red : FColor::Green;

				DrawDebugBox(World, BoundsOrigin, BoundsExtent, DebugColor, false, 5.0f, 0, 2.0f);

				DrawDebugString(
					World,
					BoundsOrigin + FVector(0.f, 0.f, BoundsExtent.Z + 30.f),
					FString::Printf(TEXT("Dot: %.3f (Min: %.3f)"), DotAngle, MinInputDotAngle),
					nullptr,
					DebugColor,
					5.0f,
					true,
					1.5f
				);

				// Draw 2D intent arrows (flattened)
				const FVector PawnPos = InstigatorPawn->GetActorLocation();

				const FVector LineEndInput  = PawnPos + FVector(InputDir2D.X,  InputDir2D.Y,  0.f) * 500.f;
				const FVector LineEndTarget = PawnPos + FVector(TargetDir2D.X, TargetDir2D.Y, 0.f) * 500.f;

				DrawDebugDirectionalArrow(World, PawnPos, LineEndInput, 50.0f,
					FColor::Yellow, false, 5.0f, 0, 3.0f);

				DrawDebugDirectionalArrow(World, PawnPos, LineEndTarget, 50.0f,
					FColor::Cyan, false, 5.0f, 0, 3.0f);
			}
		}
	}
#endif

	return bShouldFilter;
}
