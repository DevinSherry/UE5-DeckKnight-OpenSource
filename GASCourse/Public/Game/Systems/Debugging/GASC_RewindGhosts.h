#pragma once
#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
#include "UObject/StrongObjectPtr.h"
class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPoseableMeshComponent;
class UStaticMeshComponent;

struct FGASC_RewindMeshPose
{
 TStrongObjectPtr<USkeletalMesh> SkeletalMesh;
 TStrongObjectPtr<UStaticMesh> StaticMesh;
 FTransform Transform;
 TArray<FTransform> LocalBones;
};
struct FGASC_RewindActorPose
{
 TWeakObjectPtr<AActor> Actor;
 FString Name;
 FTransform Transform;
 FVector Center = FVector::ZeroVector, Extent = FVector(15);
 TArray<TSharedPtr<FGASC_RewindMeshPose>> Meshes;
 SIZE_T Bytes() const;
};
// Captures render data only, including attached actors. No gameplay actors are duplicated.
GASCOURSE_API FGASC_RewindActorPose GASC_CaptureRewindActor(AActor* Actor, bool& Truncated, double Deadline = DBL_MAX);
class GASCOURSE_API FGASC_RewindGhostRenderer
{
public:
 FGASC_RewindGhostRenderer();
 ~FGASC_RewindGhostRenderer();
 void ClearGhostMeshes();
 void Show(UWorld* World, uint64 Key, const TArray<TSharedPtr<FGASC_RewindMeshPose>>& Poses, int32 MaxMeshes);
 TArray<TWeakObjectPtr<UPoseableMeshComponent>> SkeletalPool;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> StaticPool;
 uint64 GhostKey = 0;
 TWeakObjectPtr<AActor> GhostActor;
 TStrongObjectPtr<UMaterialInterface> GhostBaseMaterial;
 TStrongObjectPtr<UMaterialInstanceDynamic> GhostMaterial;
 int32 SkippedMeshes = 0;
private:
 uint64 CreationFrame = MAX_uint64;
 int32 CreatedThisFrame = 0;
};
#endif
