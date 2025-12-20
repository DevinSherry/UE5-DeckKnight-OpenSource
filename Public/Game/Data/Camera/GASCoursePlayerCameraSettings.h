// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "InputAction.h"
#include "GASCoursePlayerCameraSettings.generated.h"

/**
 * Represents the settings and configurable parameters used for controlling the player camera in the GASCourse system.
 *
 * This class provides a centralized configuration for various camera-related settings, such as zoom distances,
 * interpolation speeds, panning behaviors, and rotation options. It enables fine-tuning of the camera behavior
 * to adapt to the desired gameplay experience in the Unreal Engine framework.
 */
UCLASS()
class GASCOURSE_API UGASCoursePlayerCameraSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float MaxCameraBoomDistance = 500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float MinCameraBoomDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float DefaultCameraBoomLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float DefaultSocketOffsetZ = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float CameraZoomDistanceStep = 10.0f;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float MinCameraPitchAngle = -10.0f;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float MaxCameraPitchAngle = 40.0f;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "GASCourse|Camera Settings", meta = (AllowPrivateAccess = "true"))
	float CameraRotationSpeedMultiplier = 1.0f;
	
};
