#include "Game/Systems/Subsystems/MeleeTrace/GASC_MeleeTrace_NotifyState.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/Skeleton.h"
#endif

namespace
{
	int32 GetMeleeMontageInstanceId(const FAnimNotifyEventReference& Reference)
	{
		const auto* Context = Reference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
		return Context ? Context->MontageInstanceID : INDEX_NONE;
	}
}

void UGASC_MeleeTrace_NotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (!IsValid(MeshComp) || !IsValid(MeshComp->GetOwner()) || !MeshComp->GetWorld() || !EventReference.GetNotify()) return;
	auto* Subsystem = MeshComp->GetWorld()->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
	if (!Subsystem) return;
	ActiveWindows.RemoveAll([](const FActiveWindow& Window) { return !Window.Mesh.IsValid(); });
	const int32 MontageId = GetMeleeMontageInstanceId(EventReference);
	// Re-entry of the same activation closes its old window; different montage instances coexist.
	for (int32 Index = ActiveWindows.Num() - 1; Index >= 0; --Index)
	{
		const auto& Window = ActiveWindows[Index];
		if (Window.Mesh == MeshComp && EventReference.GetNotify() && Window.Event == *EventReference.GetNotify()
			&& Window.Source == EventReference.GetSourceObject() && Window.MontageInstanceId == MontageId)
		{
			Subsystem->CancelMeleeTrace(Window.Id);
			ActiveWindows.RemoveAt(Index);
		}
	}

	UMeshComponent* PreviewWeapon = nullptr;
#if WITH_EDITOR
	PreviewWeapon = CreatePreviewMesh(MeshComp);
#endif
	TArray<FGASC_MeleeTrace_Subsystem_Data> Shapes;
	auto AddShape = [&](const FGASC_MeleeTrace_TraceShapeData& Row)
	{
		auto Shape = Subsystem->CreateShapeDataFromRow(Row);
		// Use the mesh that actually emitted the notify, unless a named/tagged mesh was requested.
		if (Shape.TraceObject == EGASC_MeleeTrace_TraceObject::CharacterMesh && Shape.MeshComponentNameOrTag.IsNone())
			Shape.SourceMeshComponent = MeshComp;
		if (Shape.TraceObject == EGASC_MeleeTrace_TraceObject::Weapon && PreviewWeapon &&
			(Shape.MeshComponentNameOrTag.IsNone() || PreviewWeapon->ComponentHasTag(Shape.MeshComponentNameOrTag) ||
			 PreviewWeapon->GetFName() == Shape.MeshComponentNameOrTag))
			Shape.SourceMeshComponent = PreviewWeapon;
		Shapes.Add(MoveTemp(Shape));
	};
	auto AddRow = [&](const FDataTableRowHandle& Handle)
	{
		if (Handle.IsNull()) return;
		if (const auto* Row = Handle.GetRow<FGASC_MeleeTrace_TraceShapeData>(TEXT("Melee trace notify")))
		{
			AddShape(*Row);
			if (Shapes.Last().ShapeName.IsNone()) Shapes.Last().ShapeName = Handle.RowName;
		}
	};
	AddRow(MeleeTraceRowHandle);
	for (const auto& Row : MeleeTraceRows) AddRow(Row);
	for (const auto& Shape : InlineShapes) AddShape(Shape);
	if (Shapes.IsEmpty() && !PreviewWeapon) return;

	const FGuid WindowId = FGuid::NewGuid();
	Subsystem->RequestMeleeTraceWindow(MeshComp->GetOwner(), Shapes, WindowId, HitPolicy, HitCooldown);
	Subsystem->SetNotifyPreviewWeapon(WindowId, PreviewWeapon);
	const auto* Event = EventReference.GetNotify();
	Subsystem->SetNotifyPreviewContext(WindowId, MeshComp, EventReference.GetSourceObject(), Event->GetTime(), Event->GetTime() + Event->GetDuration());
	if (Subsystem->IsMeleeTraceInProgress(WindowId) && EventReference.GetNotify())
		ActiveWindows.Add({MeshComp, *EventReference.GetNotify(), EventReference.GetSourceObject(), MontageId, WindowId});
}

void UGASC_MeleeTrace_NotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	// Runtime and animation preview use the same post-animation tracing path.
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UGASC_MeleeTrace_NotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (!MeshComp || !MeshComp->GetWorld()) return;
	auto* Subsystem = MeshComp->GetWorld()->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
	const int32 MontageId = GetMeleeMontageInstanceId(EventReference);
	for (int32 Index = ActiveWindows.Num() - 1; Index >= 0; --Index)
	{
		const auto& Window = ActiveWindows[Index];
		if (Window.Mesh == MeshComp && EventReference.GetNotify() && Window.Event == *EventReference.GetNotify()
			&& Window.Source == EventReference.GetSourceObject() && Window.MontageInstanceId == MontageId)
		{
			if (Subsystem) Subsystem->CancelMeleeTrace(Window.Id);
			ActiveWindows.RemoveAt(Index);
		}
	}
}

FString UGASC_MeleeTrace_NotifyState::GetNotifyName_Implementation() const
{
	const int32 Count = MeleeTraceRows.Num() + InlineShapes.Num() + (MeleeTraceRowHandle.IsNull() ? 0 : 1);
	return FString::Printf(TEXT("Melee: %d shapes [%s]"), Count, *StaticEnum<EGASC_MeleeHitPolicy>()->GetNameStringByValue(int64(HitPolicy)));
}

#if WITH_EDITOR
void UGASC_MeleeTrace_NotifyState::EnsurePreviewWindow(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEvent& Event)
{
	if (!MeshComp || !MeshComp->GetWorld() || MeshComp->GetWorld()->WorldType != EWorldType::EditorPreview
		|| PreviewMeshAsset.IsNull() || PreviewAttachSocket.IsNone() || !MeshComp->DoesSocketExist(PreviewAttachSocket)) return;
	auto* Subsystem = MeshComp->GetWorld()->GetSubsystem<UGASC_MeleeTrace_Subsystem>();
	if (!Subsystem) return;
	for (int32 Index = ActiveWindows.Num() - 1; Index >= 0; --Index)
	{
		const auto& Window = ActiveWindows[Index];
		if (Window.Mesh != MeshComp || !(Window.Event == Event)) continue;
		auto* Weapon = Subsystem->GetNotifyPreviewWeapon(Window.Id);
		UObject* Asset = nullptr;
		if (auto* Static = Cast<UStaticMeshComponent>(Weapon)) Asset = Static->GetStaticMesh();
		if (auto* Skeletal = Cast<USkeletalMeshComponent>(Weapon)) Asset = Skeletal->GetSkeletalMeshAsset();
		if (Weapon && Weapon->IsRegistered() && Asset == PreviewMeshAsset.Get()
			&& Weapon->GetAttachParent() == MeshComp && Weapon->GetAttachSocketName() == PreviewAttachSocket
			&& Weapon->GetRelativeTransform().Equals(PreviewRelativeTransform)
			&& (PreviewComponentTag.IsNone() || Weapon->ComponentHasTag(PreviewComponentTag))
			&& Subsystem->IsMeleeTraceInProgress(Window.Id)) return;
		Subsystem->CancelMeleeTrace(Window.Id);
		ActiveWindows.RemoveAt(Index);
	}
	NotifyBegin(MeshComp, Animation, Event.GetDuration(), FAnimNotifyEventReference(&Event, Animation));
}

TArray<FString> UGASC_MeleeTrace_NotifyState::GetPreviewAttachmentPoints() const
{
	TArray<FString> Names { TEXT("None") };
	const auto* Animation = GetTypedOuter<UAnimSequenceBase>();
	auto* Skeleton = Animation ? Animation->GetSkeleton() : nullptr;
	if (!Skeleton) return Names;
	const auto& Bones = Skeleton->GetReferenceSkeleton();
	for (int32 Index = 0; Index < Bones.GetNum(); ++Index) Names.AddUnique(Bones.GetBoneName(Index).ToString());
	for (const auto& Socket : Skeleton->Sockets)
		if (Socket) Names.AddUnique(Socket->SocketName.ToString());
	if (const auto* PreviewMesh = Skeleton->GetPreviewMesh(true))
		for (const auto* Socket : PreviewMesh->GetActiveSocketList())
			if (Socket) Names.AddUnique(Socket->SocketName.ToString());
	Names.Sort();
	return Names;
}

UMeshComponent* UGASC_MeleeTrace_NotifyState::CreatePreviewMesh(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp || !MeshComp->GetWorld() || MeshComp->GetWorld()->WorldType != EWorldType::EditorPreview || PreviewMeshAsset.IsNull()) return nullptr;
	if (!IsValid(MeshComp->GetOwner()) || PreviewAttachSocket.IsNone() || !MeshComp->DoesSocketExist(PreviewAttachSocket))
	{
		UE_LOG(LOG_GASC_MeleeTraceSubsystem, Warning, TEXT("Cannot attach melee preview mesh: select a valid character bone/socket."));
		return nullptr;
	}
	UObject* Asset = PreviewMeshAsset.LoadSynchronous();
	UMeshComponent* Weapon = nullptr;
	if (auto* StaticMesh = Cast<UStaticMesh>(Asset))
	{
		auto* Component = NewObject<UStaticMeshComponent>(MeshComp->GetOwner(), NAME_None, RF_Transient);
		Component->SetStaticMesh(StaticMesh);
		Weapon = Component;
	}
	else if (auto* SkeletalMesh = Cast<USkeletalMesh>(Asset))
	{
		auto* Component = NewObject<USkeletalMeshComponent>(MeshComp->GetOwner(), NAME_None, RF_Transient);
		Component->SetSkeletalMeshAsset(SkeletalMesh);
		Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Weapon = Component;
	}
	if (!Weapon)
	{
		UE_LOG(LOG_GASC_MeleeTraceSubsystem, Warning, TEXT("Melee preview asset must be a static or skeletal mesh."));
		return nullptr;
	}
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Weapon->SetGenerateOverlapEvents(false);
	Weapon->SetCastShadow(false);
	if (!PreviewComponentTag.IsNone()) Weapon->ComponentTags.Add(PreviewComponentTag);
	Weapon->SetupAttachment(MeshComp, PreviewAttachSocket);
	Weapon->SetRelativeTransform(PreviewRelativeTransform);
	Weapon->RegisterComponent();
	return Weapon;
}
#endif
