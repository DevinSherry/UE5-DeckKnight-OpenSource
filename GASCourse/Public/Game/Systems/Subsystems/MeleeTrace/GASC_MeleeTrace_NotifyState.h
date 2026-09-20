#pragma once

#include "GASC_MeleeTrace_Subsystem.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GASC_MeleeTrace_NotifyState.generated.h"

/** All rows in this notify form one hit window, even when attached to different meshes. */
UCLASS()
class GASCOURSE_API UGASC_MeleeTrace_NotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	/** Existing assets keep working. Included alongside the additional rows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Trace", meta=(RowType="/Script/GASCourse.GASC_MeleeTrace_TraceShapeData"))
	FDataTableRowHandle MeleeTraceRowHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Trace", meta=(RowType="/Script/GASCourse.GASC_MeleeTrace_TraceShapeData"))
	TArray<FDataTableRowHandle> MeleeTraceRows;

	/** Optional inline shapes for one-off attacks, using the same format as table rows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Trace")
	TArray<FGASC_MeleeTrace_TraceShapeData> InlineShapes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Trace|Hits")
	EGASC_MeleeHitPolicy HitPolicy = EGASC_MeleeHitPolicy::UseProjectDefault;

	/** -1 uses project settings. Only used by RehitAfterDelay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Trace|Hits", meta=(ClampMin="-1"))
	float HitCooldown = -1.0f;

#if WITH_EDITORONLY_DATA
	/** Weapon used only in animation preview worlds; never equips a gameplay weapon. */
	UPROPERTY(EditAnywhere, Category="Melee Trace|Editor Preview", meta=(AllowedClasses="/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
	TSoftObjectPtr<UObject> PreviewMeshAsset;
	/** Bone/socket on the animation's character skeleton, not on the preview weapon. */
	UPROPERTY(EditAnywhere, Category="Melee Trace|Editor Preview", meta=(GetOptions="GetPreviewAttachmentPoints"))
	FName PreviewAttachSocket;
	UPROPERTY(EditAnywhere, Category="Melee Trace|Editor Preview")
	FTransform PreviewRelativeTransform = FTransform::Identity;
	/** Optional tag allowing shape rows to explicitly select this preview weapon. */
	UPROPERTY(EditAnywhere, Category="Melee Trace|Editor Preview")
	FName PreviewComponentTag = TEXT("MeleePreviewWeapon");
#endif

#if WITH_EDITOR
	UFUNCTION()
	TArray<FString> GetPreviewAttachmentPoints() const;
	UMeshComponent* CreatePreviewMesh(USkeletalMeshComponent* MeshComp) const;
	/** Restore the active preview after a seek or Persona attachment cleanup. */
	void EnsurePreviewWindow(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent& Event);
#endif

private:
	// Notify objects belong to animation assets, not actors. Store activation identity separately.
	struct FActiveWindow
	{
		TWeakObjectPtr<USkeletalMeshComponent> Mesh;
		// Queued notifies pass a copy of the asset event to NotifyEnd.
		FAnimNotifyEvent Event;
		TWeakObjectPtr<const UObject> Source;
		int32 MontageInstanceId = INDEX_NONE;
		FGuid Id;
	};
	TArray<FActiveWindow> ActiveWindows;
};
