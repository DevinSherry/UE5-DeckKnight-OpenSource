#include "Game/Systems/Debugging/Panels/FGASC_TargetingDebugPanel.h"
#if !UE_BUILD_SHIPPING
#include "Types/TargetingEvaluationDebug.h"
#include "Game/Systems/Debugging/GASC_RewindGhosts.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Tasks/TargetingTask.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "Tasks/TargetingTask_Conditional.h"
#include "Types/TargetingExecutionPlan.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortDistance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "imgui.h"
#include "Game/Systems/Debugging/GASC_RewindDebugControls.h"
#include "Misc/ScopeExit.h"
#include "HAL/PlatformTime.h"

static_assert(ENABLE_DRAW_DEBUG, "Targeting history requires UE_ENABLE_DEBUG_DRAWING=1 in non-Shipping targets.");

namespace GASCTargetingHistory
{
	constexpr int32 MaxTargets = 256, MaxStages = 64, MaxRecords = 128;
	constexpr SIZE_T MaxBytes = 64 * 1024 * 1024;
	static bool ScoreDescending(float A, float B)
	{
		return FMath::IsFinite(A) != FMath::IsFinite(B) ? FMath::IsFinite(A) : FMath::IsFinite(A) && A > B;
	}
	using FMeshPose = FGASC_RewindMeshPose;
	struct FTarget
	{
		FTargetingDefaultResultData Result;
		FString Name, ClassName, Info;
		FTransform Transform;
		FVector LocalCenter = FVector::ZeroVector, Extent = FVector(15);
		
		TArray<TSharedPtr<FMeshPose>> Meshes;
		float Before = 0, Added = 0;
		bool bRemoved = false, bAcquired = false, bUnknown = false, bActorHit = false;
		SIZE_T Bytes() const
		{
			SIZE_T Size = sizeof(*this) + Name.GetAllocatedSize() + ClassName.GetAllocatedSize() + Info.GetAllocatedSize() + Meshes.GetAllocatedSize();
			for (const auto& M : Meshes) Size += sizeof(FMeshPose) + M->LocalBones.GetAllocatedSize();
			return Size;
		}
	};
	struct FStage
	{
		FString Name;
		uint64 Frame = 0, EndFrame = 0;
		FTarget SourceGhost, InstigatorGhost;
		bool bDraw = false, bFilter = false, bScoring = false, bDistance = false, bCondition = false;
		int32 ExecutionIndex = INDEX_NONE, ParentStage = INDEX_NONE;
		FString Summary, ChildrenDescription;
		bool bBeforeTruncated = false;
		bool bCompleted = false;
		TArray<FTarget> Before, Rows;
		TArray<FTargetingDebugPrimitive> Primitives;
	};
	struct FEvaluation
	{
		uint64 Id = 0, Frame = 0, EndFrame = 0;
		double Time = 0;
		FString Preset, Source, Status;
		TArray<FStage> Stages;
		TArray<FTarget> Final;
		bool bTruncated = false;
		SIZE_T RetainedBytes = 0, StageBytes = 0;
		SIZE_T Bytes() const
		{
			SIZE_T Size = sizeof(*this);
			for (const FStage& S : Stages) { Size += sizeof(S) + S.Name.GetAllocatedSize() + S.Summary.GetAllocatedSize() + S.ChildrenDescription.GetAllocatedSize() + S.Primitives.GetAllocatedSize() + S.SourceGhost.Bytes() + S.InstigatorGhost.Bytes(); for (const FTarget& T : S.Before) Size += T.Bytes(); for (const FTarget& T : S.Rows) Size += T.Bytes(); }
			for (const FTarget& T : Final) Size += T.Bytes();
			return Size;
		}
	};
	static FString QueryExpression(const FGameplayTagQueryExpression& Expr, const FGameplayTagContainer& Tags, int32 Depth = 0)
	{
		if (Depth > 16) return TEXT("[expression depth limit]");
		const bool Exact = Expr.ExprType == EGameplayTagQueryExprType::AnyTagsExactMatch || Expr.ExprType == EGameplayTagQueryExprType::AllTagsExactMatch;
		TArray<FString> Parts;
		for (const auto& Tag : Expr.TagSet)
		{
			if (Parts.Num() >= 32) { Parts.Add(TEXT("...")); break; }
			Parts.Add(Tag.ToString() + ((Exact ? Tags.HasTagExact(Tag) : Tags.HasTag(Tag)) ? TEXT("=present") : TEXT("=absent")));
		}
		for (const auto& Child : Expr.ExprSet)
		{
			if (Parts.Num() >= 32) { Parts.Add(TEXT("...")); break; }
			Parts.Add(QueryExpression(Child, Tags, Depth + 1));
		}
		return (StaticEnum<EGameplayTagQueryExprType>()->GetNameStringByValue(int64(Expr.ExprType)) + TEXT("(") + FString::Join(Parts, TEXT(", ")) + TEXT(")")).Left(3000);
	}
	static FString TargetInfo(const UTargetingTask* Task, const FTargetingRequestHandle& Handle, const FTargetingDefaultResultData& Target, bool Filtered)
	{
		// Preserve Blueprint overrides; augment the matches-tag-query Blueprint's reflected query.
		FString Info = Task->GetEvaluationDebugTargetInfo(Handle, Target, Filtered);
		const auto* Property = FindFProperty<FStructProperty>(Task->GetClass(), TEXT("TagQuery"));
		if (!Task->IsA<UTargetingFilterTask_BasicFilterTemplate>() || !Property || Property->Struct != FGameplayTagQuery::StaticStruct()) return Info;
		const auto& Query = *Property->ContainerPtrToValuePtr<FGameplayTagQuery>(Task);
		const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target.HitResult.GetActor());
		FGameplayTagContainer Tags;
		if (ASC) ASC->GetOwnedGameplayTags(Tags);
		FGameplayTagQueryExpression Expr; Query.GetQueryExpr(Expr);
		FString Reason = FString::Printf(TEXT("Query: %s; expression: %s; %s; outcome: %s"),
			*Query.GetDescription(), *QueryExpression(Expr, Tags),
			ASC ? (Query.Matches(Tags) ? TEXT("query matched owned tags") : TEXT("query did not match owned tags")) : TEXT("target has no ability system component; query input unavailable"),
			Filtered ? TEXT("Filtered") : TEXT("Not Filtered"));
		if (!Info.IsEmpty() && Info != Task->UTargetingTask::GetEvaluationDebugTargetInfo_Implementation(Handle, Target, Filtered)) Reason += TEXT("; processor: ") + Info;
		return Reason;
	}

	static FTarget Snapshot(const FTargetingDefaultResultData& Result, int32 Detail = 0, double Deadline = DBL_MAX)
	{
		FTarget T;
		T.Result = Result;
		T.bActorHit = Result.HitResult.HasValidHitObjectHandle();
		T.Before = Result.Score;
		T.Transform = FTransform(Result.HitResult.Location);
		T.Name = TEXT("Location-only hit");
		if (AActor* Actor = Result.HitResult.GetActor())
		{
			T.Name = Actor->GetName(); T.ClassName = Actor->GetClass()->GetName(); T.Transform = Actor->GetActorTransform();
			const FBox Box = Actor->CalculateComponentsBoundingBoxInLocalSpace();
			if (Box.IsValid) { T.LocalCenter = Box.GetCenter(); T.Extent = Box.GetExtent(); }
			if (Detail > 0)
			{
			bool Truncated = false; T.Meshes = GASC_CaptureRewindActor(Actor, Truncated, Deadline).Meshes;
			}
		}
		return T;
	}
	static TArray<FTarget> SnapshotResults(FTargetingRequestHandle Handle, bool& Truncated, int32 Detail = 0, double Deadline = DBL_MAX)
	{
		TArray<FTarget> Targets;
		SIZE_T Bytes = 0;
		if (const auto* Results = FTargetingDefaultResultsSet::Find(Handle))
		{
			Truncated |= Results->TargetResults.Num() > MaxTargets;
			for (int32 I = 0; I < FMath::Min(Results->TargetResults.Num(), MaxTargets); ++I)
			{
				if (FPlatformTime::Seconds() >= Deadline) { Truncated = true; break; }
				FTarget Target = Snapshot(Results->TargetResults[I], Detail, Deadline); Bytes += Target.Bytes();
				if (Bytes > MaxBytes / 32) { Truncated = true; break; }
				Targets.Add(MoveTemp(Target));
			}
		}
		return Targets;
	}
	static bool SameHit(const FTarget& A, const FTarget& B)
	{
		return A.Result.HitResult.HitObjectHandle == B.Result.HitResult.HitObjectHandle && A.Result.HitResult.Component == B.Result.HitResult.Component && A.Result.HitResult.Item == B.Result.HitResult.Item && A.Result.HitResult.BoneName == B.Result.HitResult.BoneName && ((A.bActorHit && B.bActorHit) || A.Result.HitResult.Location.Equals(B.Result.HitResult.Location));
	}
	static TArray<const FTarget*> ProcessedActors(const FEvaluation& E)
	{
		TArray<const FTarget*> Out; TSet<FString> Names;
		auto Add = [&](const FTarget& T) { if (!T.Name.IsEmpty() && !Names.Contains(T.Name)) { Names.Add(T.Name); Out.Add(&T); } };
		for (const auto& T : E.Final) Add(T);
		for (int32 I = E.Stages.Num() - 1; I >= 0; --I) for (const auto& T : E.Stages[I].Rows) Add(T);
		return Out;
	}
	static FString FinalLabel(const FEvaluation& E, const FTarget& T)
	{
		const auto* Final = E.Final.FindByPredicate([&](const FTarget& Row) { return SameHit(Row, T); });
		if (Final) return FString::Printf(TEXT("Final score: %.4f"), Final->Result.Score);
		return E.bTruncated || T.bUnknown ? TEXT("Final score: unavailable (incomplete capture)") : FString::Printf(TEXT("Filtered | final score: -- | last score %.4f"), T.Result.Score);
	}

	static FString WorldLabel(const FString& Text)
	{
		// Bound glyph submissions as well as debug lines. Full details remain in the ImGui row.
		FString Label = Text.Left(768); if (Text.Len() > 768) Label += TEXT("... [full details in ImGui]");
		int32 Column = 0;
		for (TCHAR& Character : Label)
		{
			if (Character == '\n') Column = 0;
			else if (++Column >= 80 && Character == ' ') { Character = '\n'; Column = 0; }
		}
		return Label;
	}
	static void DrawGhost(UWorld* World, const FTarget& T, FColor Color, int32& LinesLeft, bool Labels, const FString& Details = FString())
	{
		if (T.Name.IsEmpty() || LinesLeft < 24) return;
		LinesLeft -= 24;
		DrawDebugBox(World, T.Transform.TransformPosition(T.LocalCenter), T.Extent * T.Transform.GetScale3D().GetAbs(), T.Transform.GetRotation(), Color, false, -1, 0, 1);
		DrawDebugDirectionalArrow(World, T.Transform.GetLocation(), T.Transform.GetLocation() + T.Transform.GetUnitAxis(EAxis::X) * 70, 15, Color, false, -1, 0, 2);
		
		if (Labels) DrawDebugString(World, T.Transform.TransformPosition(T.LocalCenter) + FVector(0, 0, (T.Extent * T.Transform.GetScale3D()).Size() + 90), WorldLabel(T.Name + TEXT("\n") + (Details.IsEmpty() ? T.ClassName : Details)), nullptr, Color, 0, true);
	}
	static void DrawPrimitive(UWorld* World, const FTargetingDebugPrimitive& P)
	{
		const FVector Origin = P.Transform.GetLocation(); const FColor Color = P.Color.ToFColor(true);
		switch (P.Shape)
		{
		case ETargetingDebugShape::Arrow: DrawDebugDirectionalArrow(World, Origin, P.Vector, 65, Color, false, -1, 0, 5); break;
		case ETargetingDebugShape::Line: DrawDebugLine(World, Origin, P.Vector, Color, false, -1, 0, 2); break;
		case ETargetingDebugShape::Circle: DrawDebugCircle(World, Origin, P.Radius, 24, Color, false, -1, 0, 2, FVector::ForwardVector, FVector::RightVector, false); break;
		case ETargetingDebugShape::Box: DrawDebugBox(World, Origin, P.Vector, P.Transform.GetRotation(), Color, false, -1, 0, 2); break;
		case ETargetingDebugShape::Sphere: DrawDebugSphere(World, Origin, P.Radius, 12, Color, false, -1, 0, 2); break;
		case ETargetingDebugShape::Capsule: DrawDebugCapsule(World, Origin, P.Vector.Z, P.Radius, P.Transform.GetRotation(), Color, false, -1, 0, 2); break;
		case ETargetingDebugShape::Cylinder: DrawDebugCylinder(World, Origin - P.Transform.GetUnitAxis(EAxis::Z) * P.Vector.Z, Origin + P.Transform.GetUnitAxis(EAxis::Z) * P.Vector.Z, P.Radius, 16, Color, false, -1, 0, 2); break;
		}
	}
	static const TCHAR* Outcome(const FTarget& T, bool Filter)
	{
		return T.bUnknown ? TEXT("Unknown (limited capture)") : T.bRemoved ? TEXT("Filtered") : Filter ? TEXT("Not Filtered") : T.bAcquired ? TEXT("Acquired") : TEXT("Retained");
	}
	static void Table(const TArray<FTarget>& Rows, bool Final, bool Filter = false, bool Scoring = true)
	{
		if (Final)
		{
			if (!ImGui::BeginTable("FinalTargets", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, FMath::Min(Rows.Num() + 1, 12) * ImGui::GetTextLineHeightWithSpacing() + 12))) return;
			for (const char* Header : {"Rank", "Target", "Final score"}) ImGui::TableSetupColumn(Header);
			ImGui::TableHeadersRow();
			ImGuiListClipper Clipper; Clipper.Begin(Rows.Num());
		while (Clipper.Step()) for (int32 I = Clipper.DisplayStart; I < Clipper.DisplayEnd; ++I)
			{
				const bool Winner = FMath::IsFinite(Rows[I].Result.Score) && Rows[I].Result.Score == Rows[0].Result.Score;
				if (Winner) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.2f, 1.f, .3f, 1.f));
				ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("%d", I + 1);
				ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(*Rows[I].Name));
				ImGui::TableNextColumn(); ImGui::Text("%.4f", Rows[I].Result.Score);
				if (Winner) ImGui::PopStyleColor();
			}
			ImGui::EndTable(); return;
		}
		if (!ImGui::BeginTable("Targets", Scoring ? 6 : 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, FMath::Min(Rows.Num() + 1, 12) * ImGui::GetTextLineHeightWithSpacing() + 12))) return;
		ImGui::TableSetupColumn("Target"); ImGui::TableSetupColumn("Outcome");
		if (Scoring) for (const char* Header : {"Before", "Added", "Total"}) ImGui::TableSetupColumn(Header);
		ImGui::TableSetupColumn("Details / reason");
		ImGui::TableHeadersRow();
		ImGuiListClipper Clipper; Clipper.Begin(Rows.Num());
		while (Clipper.Step()) for (int32 I = Clipper.DisplayStart; I < Clipper.DisplayEnd; ++I)
		{
			const FTarget& T = Rows[I];
			const bool Winner = Final && T.Result.Score == Rows[0].Result.Score;
			if (Winner) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.f, 0.3f, 1.f));
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(*T.Name));
			ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(Outcome(T, Filter)));
			if (Scoring)
			{
			ImGui::TableNextColumn(); if (T.bUnknown) ImGui::TextUnformatted("--"); else ImGui::Text("%.4f", T.Before);
			ImGui::TableNextColumn(); if (T.bUnknown) ImGui::TextUnformatted("--"); else ImGui::Text("%+.4f", T.Added);
			ImGui::TableNextColumn(); if (T.bRemoved) ImGui::TextUnformatted("--"); else ImGui::Text("%.4f", T.Result.Score);
			}
			ImGui::TableNextColumn(); ImGui::TextUnformatted(TCHAR_TO_UTF8(*T.Info.Replace(TEXT("\n"), TEXT(" ")).Replace(TEXT("\r"), TEXT(" "))));
			if (ImGui::IsItemHovered()) { ImGui::BeginTooltip(); ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40); ImGui::TextUnformatted(TCHAR_TO_UTF8(*T.Info)); ImGui::PopTextWrapPos(); ImGui::EndTooltip(); }
			if (Winner) ImGui::PopStyleColor();
		}
		ImGui::EndTable();
	}
}

class FGASC_TargetingDebugRecorder : public FGASC_RewindGhostRenderer
{
public:
	using FEvaluation = GASCTargetingHistory::FEvaluation;
	explicit FGASC_TargetingDebugRecorder(UGameInstance* InInstance) : World(InInstance ? InInstance->GetWorld() : nullptr), Instance(InInstance)
	{
#if !UE_BUILD_SHIPPING
		EventHandle = FTargetingEvaluationDebug::OnEvent().AddRaw(this, &FGASC_TargetingDebugRecorder::OnEvent);
		PrimitiveHandle = FTargetingEvaluationDebug::OnPrimitive().AddLambda([this](FTargetingRequestHandle H, const FTargetingDebugPrimitive& P)
		{
			if (FEvaluation* E = Pending.Find(H.Handle)) if (!E->Stages.IsEmpty())
			{
				if (E->Stages.Last().Primitives.Num() < 512) E->Stages.Last().Primitives.Add(P); else E->bTruncated = true;
			}
		});
		ReleaseHandle = FTargetingRequestHandle::GetReleaseHandleDelegate().AddLambda([this](FTargetingRequestHandle Handle)
		{
			if (FEvaluation* E = Pending.Find(Handle.Handle)) { E->Status = TEXT("Released / cancelled before completion"); Commit(Handle.Handle); }
		});
#endif
	}
	~FGASC_TargetingDebugRecorder()
	{
		FTargetingEvaluationDebug::OnEvent().Remove(EventHandle);
		FTargetingEvaluationDebug::OnPrimitive().Remove(PrimitiveHandle);
		FTargetingRequestHandle::GetReleaseHandleDelegate().Remove(ReleaseHandle);
		ClearGhostMeshes();
	}
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<UGameInstance> Instance;
	FDelegateHandle EventHandle, ReleaseHandle, PrimitiveHandle;
	TSet<FString> ViewPresets;
	TMap<uint32, FEvaluation> Pending;
	TArray<FEvaluation> History;
	SIZE_T HistoryBytes = 0;
	TMap<const FTargetingTaskSet*, TWeakObjectPtr<UTargetingPreset>> PresetCache;
	bool bRecording = true, bFollow = true, bGhosts = true;
	int32 CaptureDetail = 1, LineBudget = 512, MaxDrawTargets = GASCTargetingHistory::MaxTargets;
	bool bWorldLabels = true, bShowMeshGhosts = true;
	float CaptureBudgetMs = 1.f;
	uint64 CaptureFrame = MAX_uint64;
	double CaptureFrameMs = 0, LastCaptureMs = 0, PeakCaptureMs = 0, PanelMs = 0, PeakPanelMs = 0;
	double Deadline = DBL_MAX;
	int32 BudgetDrops = 0, LinesLeft = 0, TargetsLeft = 0, SkippedDraws = 0;
	uint64 NextId = 1, SelectedId = 0;
	int32 Dropped = 0;
	char Search[256] = {};
	TArray<FString> AvailablePresets;
	bool bPresetsLoaded = false;
	void RefreshPresets()
	{
		TSet<FString> Paths;
		FARFilter Filter; Filter.ClassPaths.Add(UTargetingPreset::StaticClass()->GetClassPathName()); Filter.bRecursiveClasses = true;
		TArray<FAssetData> Assets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Assets);
		for (const auto& Asset : Assets) Paths.Add(Asset.GetSoftObjectPath().ToString());
		for (TObjectIterator<UTargetingPreset> It; It; ++It) if (!It->HasAnyFlags(RF_ClassDefaultObject)) Paths.Add(It->GetPathName());
		AvailablePresets = Paths.Array(); AvailablePresets.Sort();
		bPresetsLoaded = true;
	}
	void RefreshWorld()
	{
		if (Instance.IsValid() && World.Get() != Instance->GetWorld()) { ClearGhostMeshes(); Pending.Empty(); History.Empty(); HistoryBytes = 0; PresetCache.Empty(); World = Instance->GetWorld(); }
	}
	void DrawTarget(const GASCTargetingHistory::FTarget& T, FColor Color, const FString& Details = FString())
	{
		if (T.Name.IsEmpty()) return;
		if (TargetsLeft <= 0 || LinesLeft < 24) { ++SkippedDraws; return; }
		--TargetsLeft; GASCTargetingHistory::DrawGhost(World.Get(), T, Color, LinesLeft, bWorldLabels, Details);
	}
	void DrawShape(const FTargetingDebugPrimitive& P)
	{
		int32 Cost = 24;
		switch (P.Shape)
		{
		case ETargetingDebugShape::Line: Cost = 1; break;
		case ETargetingDebugShape::Box: Cost = 12; break;
		case ETargetingDebugShape::Sphere: Cost = 400; break;
		case ETargetingDebugShape::Capsule: Cost = 256; break;
		case ETargetingDebugShape::Cylinder: Cost = 64; break;
		default: break;
		}
		if (LinesLeft < Cost) { ++SkippedDraws; return; }
		LinesLeft -= Cost; GASCTargetingHistory::DrawPrimitive(World.Get(), P);
	}
	void DrawDistances(const GASCTargetingHistory::FStage& Stage)
	{
		if (!Stage.bDistance) return;
		const auto& Anchor = Stage.InstigatorGhost.Name.IsEmpty() ? Stage.SourceGhost : Stage.InstigatorGhost;
		if (Anchor.Name.IsEmpty()) return;
		float Min = FLT_MAX, Max = -FLT_MAX;
		for (const auto& T : Stage.Rows) if (!T.bUnknown && FMath::IsFinite(T.Added)) { Min = FMath::Min(Min, T.Added); Max = FMath::Max(Max, T.Added); }
		int32 Count = 0;
		for (const auto& T : Stage.Rows)
		{
			if (++Count > MaxDrawTargets || LinesLeft < 1) { ++SkippedDraws; continue; }
			const FVector Start = Anchor.Transform.GetLocation(), End = T.Transform.GetLocation();
			const float Quality = Stage.bScoring ? (Max > Min ? (T.Added - Min) / (Max - Min) : 1.f) : T.bRemoved ? 0.f : 1.f;
			const FColor Color = T.bUnknown || !FMath::IsFinite(Quality) ? FColor::Silver : FMath::Lerp(FLinearColor::Red, FLinearColor::Green, FMath::Clamp(Quality, 0.f, 1.f)).ToFColor(true);
			--LinesLeft; DrawDebugLine(World.Get(), Start, End, Color, false, -1, 0, 2);
			DrawDebugString(World.Get(), (Start + End) * .5f + FVector(0, 0, 20), FString::Printf(TEXT("%.1f uu"), FVector::Distance(Start, End)), nullptr, Color, 0, true);
		}
	}

	void ShowGhostMeshes(const FEvaluation& E)
 {
  if (!bShowMeshGhosts) { ClearGhostMeshes(); return; }
  uint64 Key = HashCombine(GetTypeHash(E.Id), GetTypeHash(MaxDrawTargets));
  for (const auto& S : E.Stages) Key = HashCombine(Key, GetTypeHash(S.bDraw));
  if (GhostActor.IsValid() && GhostKey == Key && Key != 0) return;
  TArray<TSharedPtr<FGASC_RewindMeshPose>> Poses; TSet<FString> Seen;
  auto Add = [&](const GASCTargetingHistory::FTarget& T) { if (!T.Meshes.IsEmpty() && !Seen.Contains(T.Name) && Seen.Num() < MaxDrawTargets + 2) { Seen.Add(T.Name); Poses.Append(T.Meshes); } };
  if (!E.Stages.IsEmpty()) { Add(E.Stages.Last().SourceGhost); Add(E.Stages.Last().InstigatorGhost); }
  for (const auto* T : GASCTargetingHistory::ProcessedActors(E)) Add(*T);
  Show(World.Get(), Key, Poses, (MaxDrawTargets + 2) * 8); SkippedDraws += SkippedMeshes;
 }

	void Commit(uint32 Handle)
	{
		FEvaluation E; if (!Pending.RemoveAndCopyValue(Handle, E)) return;
		E.EndFrame = GFrameCounter;
		for (auto& Stage : E.Stages) if (!Stage.bCompleted)
		{
			Stage.Rows = MoveTemp(Stage.Before);
			for (auto& Row : Stage.Rows) { Row.bUnknown = true; Row.Info = TEXT("Processor did not complete; showing incoming targets."); }
		}
		E.RetainedBytes = E.Bytes(); HistoryBytes += E.RetainedBytes;
		History.Add(MoveTemp(E));
		History.StableSort([](const FEvaluation& A, const FEvaluation& B) { return A.Frame != B.Frame ? A.Frame < B.Frame : A.Id < B.Id; });
		while (History.Num() > GASCTargetingHistory::MaxRecords || (HistoryBytes > GASCTargetingHistory::MaxBytes && History.Num() > 1)) { HistoryBytes -= History[0].RetainedBytes; History.RemoveAt(0); ++Dropped; }
	}
	void OnEvent(FTargetingRequestHandle Handle, const UTargetingTask* Task, ETargetingEvaluationDebugEvent Event)
	{
		using namespace GASCTargetingHistory;
		if (!bRecording && Pending.IsEmpty()) return;
		if (Event != ETargetingEvaluationDebugEvent::BeginRequest && !Pending.Contains(Handle.Handle)) return;
		if (Event == ETargetingEvaluationDebugEvent::BeginRequest && !bRecording) return;
		const double Started = FPlatformTime::Seconds();
		if (CaptureFrame != GFrameCounter) { LastCaptureMs = CaptureFrameMs; CaptureFrameMs = 0; CaptureFrame = GFrameCounter; }
		ON_SCOPE_EXIT { CaptureFrameMs += (FPlatformTime::Seconds() - Started) * 1000; PeakCaptureMs = FMath::Max(PeakCaptureMs, CaptureFrameMs); };
		Deadline = Started + FMath::Max(0.0, double(CaptureBudgetMs) - CaptureFrameMs) / 1000;
		if (CaptureFrameMs >= CaptureBudgetMs)
		{
			if (auto* Limited = Pending.Find(Handle.Handle)) { Limited->bTruncated = true; Limited->Status = TEXT("Capture CPU budget reached"); Commit(Handle.Handle); ++BudgetDrops; }
			else if (Event == ETargetingEvaluationDebugEvent::BeginRequest) ++BudgetDrops;
			return;
		}
		RefreshWorld();
		if (!World.IsValid()) return;
		if (Event == ETargetingEvaluationDebugEvent::BeginRequest)
		{
			if (!bRecording) return;
			const auto* Source = FTargetingSourceContext::Find(Handle);
			UWorld* SourceWorld = Source && Source->SourceActor ? Source->SourceActor->GetWorld() : Source && Source->SourceObject ? Source->SourceObject->GetWorld() : nullptr;
			if (!SourceWorld) if (const auto* Request = FTargetingRequestData::Find(Handle)) if (Request->TargetingSubsystem) SourceWorld = Request->TargetingSubsystem->GetWorld();
			if (SourceWorld != World.Get()) return;
			const auto* OriginalTaskSet = FTargetingExecutionPlan::GetOriginalTaskSet(Handle);
			const auto* TaskSet = &OriginalTaskSet;
			if (!OriginalTaskSet) return;
			UTargetingPreset* Preset = PresetCache.FindRef(*TaskSet).Get();
			if (!Preset && !(*TaskSet)->Tasks.IsEmpty() && (*TaskSet)->Tasks[0]) Preset = (*TaskSet)->Tasks[0]->GetTypedOuter<UTargetingPreset>();
			if (!Preset || Preset->GetTargetingTaskSet() != *TaskSet)
			{
				Preset = nullptr;
				for (TObjectIterator<UTargetingPreset> It; It; ++It) if (It->GetTargetingTaskSet() == *TaskSet) { Preset = *It; break; }
			}
			if (Preset) { if (PresetCache.Num() >= 256) PresetCache.Reset(); PresetCache.Add(*TaskSet, Preset); }
			if (Preset)
			{
				if (Pending.Num() >= 16) { ++Dropped; return; }
				FEvaluation& E = Pending.FindOrAdd(Handle.Handle); E = FEvaluation();
				E.Id = NextId++; E.Frame = GFrameCounter; E.Time = World->GetTimeSeconds(); E.Preset = Preset->GetPathName(); E.Source = GetNameSafe(Source ? Source->SourceActor.Get() : nullptr);
				return;
			}
			return;
		}
		FEvaluation* E = Pending.Find(Handle.Handle); if (!E) return;
		if (Event == ETargetingEvaluationDebugEvent::CancelRequest) { E->Status = TEXT("Cancelled"); Commit(Handle.Handle); return; }
		if (Event == ETargetingEvaluationDebugEvent::BeginTask && Task)
		{
			if (E->Stages.Num() >= MaxStages || E->StageBytes > MaxBytes / 16) { E->bTruncated = true; E->Status = TEXT("Capture limit reached"); Commit(Handle.Handle); return; }
			FStage& S = E->Stages.AddDefaulted_GetRef(); S.Name = Task->GetName() + TEXT(" [") + Task->GetClass()->GetName() + TEXT("]"); S.Frame = GFrameCounter; S.bDraw = Task->bVisualizeEvaluation; S.bFilter = Task->IsA<UTargetingFilterTask_BasicFilterTemplate>();
			S.bScoring = Task->IsA<UTargetingSortTask_Base>();
			S.bDistance = Task->bVisualizeDistanceToTargets || Task->IsA<UGASCourse_TargetSortDistance>();
			for (const UClass* Class = Task->GetClass(); Class && !S.bDistance; Class = Class->GetSuperClass()) S.bDistance = Class->GetFName() == TEXT("TargetingFilterTask_SortByDistance");
			S.ExecutionIndex = FTargetingExecutionPlan::GetTaskIndex(Handle, Task);
			const int32 ParentIndex = FTargetingExecutionPlan::GetParentIndex(Handle, S.ExecutionIndex);
			if (ParentIndex != INDEX_NONE) S.ParentStage = E->Stages.IndexOfByPredicate([&](const FStage& Stage) { return Stage.ExecutionIndex == ParentIndex; });
			if (const auto* Condition = Cast<UTargetingTask_Conditional>(Task))
			{
				S.bCondition = true; S.Name = TEXT("Condition: ") + Condition->Label + TEXT(" (Children)");
				for (const UTargetingTask* Child : Condition->Children) S.ChildrenDescription += TEXT("Child: ") + GetNameSafe(Child) + TEXT("\n");
			}
			S.Before = SnapshotResults(Handle, S.bBeforeTruncated, 0, Deadline); E->bTruncated |= S.bBeforeTruncated;
			if (const auto* Source = FTargetingSourceContext::Find(Handle))
			{
				auto CaptureActor = [&](AActor* Actor, FTarget& Ghost)
				{
					if (!Actor) return;
					FTargetingDefaultResultData Result; Result.HitResult = FHitResult(Actor, nullptr, Actor->GetActorLocation(), FVector::UpVector);
					Ghost = Snapshot(Result, CaptureDetail, Deadline);
				};
				CaptureActor(Source->SourceActor, S.SourceGhost);
				if (Source->InstigatorActor != Source->SourceActor) CaptureActor(Source->InstigatorActor, S.InstigatorGhost);
			}
		}
		else if (Event == ETargetingEvaluationDebugEvent::EndTask && Task && !E->Stages.IsEmpty())
		{
			if (E->Stages.Last().bCompleted) return;
			FStage& S = E->Stages.Last(); bool AfterTruncated = false; S.Rows = SnapshotResults(Handle, AfterTruncated, CaptureDetail, Deadline); E->bTruncated |= AfterTruncated;
			S.bCompleted = true;
			S.EndFrame = GFrameCounter;
			if (S.bCondition) S.Summary = FTargetingExecutionPlan::GetConditionSummary(Handle, Task);
			if (AfterTruncated || S.bBeforeTruncated)
			{
				for (FTarget& Row : S.Rows) { Row.bUnknown = true; Row.Info = TEXT("Comparison unavailable: target snapshot limit exceeded."); }
				S.Before.Empty(); E->Status = TEXT("Capture limit reached"); Commit(Handle.Handle); return;
			}
			TBitArray<> Used(false, S.Before.Num());
			for (FTarget& Row : S.Rows)
			{
				if (FPlatformTime::Seconds() >= Deadline) { E->bTruncated = true; E->Status = TEXT("Capture CPU budget reached"); for (auto& Unknown : S.Rows) { Unknown.bUnknown = true; Unknown.Info = TEXT("CPU capture limit; comparison incomplete."); } S.Before.Empty(); Commit(Handle.Handle); ++BudgetDrops; return; }
				int32 Match = INDEX_NONE;
				for (int32 I = 0; I < S.Before.Num(); ++I) if (!Used[I] && SameHit(Row, S.Before[I])) { Match = I; Used[I] = true; break; }
				Row.bAcquired = Match == INDEX_NONE; Row.Before = Row.bAcquired ? 0 : S.Before[Match].Result.Score; Row.Added = Row.Result.Score - Row.Before;
				Row.Info = TargetInfo(Task, Handle, Row.Result, false).Left(4096);
			}
			for (int32 I = 0; I < S.Before.Num(); ++I) if (!Used[I])
			{
				if (FPlatformTime::Seconds() >= Deadline) { E->bTruncated = true; E->Status = TEXT("Capture CPU budget reached during filter details"); S.Before.Empty(); Commit(Handle.Handle); ++BudgetDrops; return; }
				FTarget Row = S.Before[I].Result.HitResult.GetActor() ? Snapshot(S.Before[I].Result, CaptureDetail, Deadline) : S.Before[I];
				Row.bRemoved = true; Row.Info = TargetInfo(Task, Handle, Row.Result, true).Left(4096); S.Rows.Add(MoveTemp(Row));
			}
			S.Before.Empty();
			S.Rows.StableSort([](const FTarget& A, const FTarget& B) { return ScoreDescending(A.Added, B.Added); });
			if (FPlatformTime::Seconds() >= Deadline) { E->bTruncated = true; E->Status = TEXT("Capture CPU budget reached before custom visuals"); Commit(Handle.Handle); ++BudgetDrops; return; }
			S.Primitives.Append(Task->GetEvaluationDebugPrimitives(Handle));
			if (S.Primitives.Num() > 512) { S.Primitives.SetNum(512); E->bTruncated = true; }
			E->StageBytes += sizeof(S) + S.Summary.GetAllocatedSize() + S.ChildrenDescription.GetAllocatedSize() + S.Primitives.GetAllocatedSize() + S.SourceGhost.Bytes() + S.InstigatorGhost.Bytes();
			for (const auto& Row : S.Rows) E->StageBytes += Row.Bytes();
			if (E->StageBytes > MaxBytes / 16) { E->bTruncated = true; E->Status = TEXT("Capture memory limit reached"); Commit(Handle.Handle); }
		}
		else if (Event == ETargetingEvaluationDebugEvent::EndRequest)
		{
			// Reuse completion-time poses: an async task may finish before the subsystem next ticks.
			if (!E->Stages.IsEmpty() && E->Stages.Last().bCompleted) E->Final = E->Stages.Last().Rows.FilterByPredicate([](const FTarget& T) { return !T.bRemoved; });
			else E->Final = SnapshotResults(Handle, E->bTruncated);
			E->Final.StableSort([](const FTarget& A, const FTarget& B) { return ScoreDescending(A.Result.Score, B.Result.Score); });
			E->Status = TEXT("Completed"); Commit(Handle.Handle);
		}
	}
};

FGASC_TargetingDebugPanel::FGASC_TargetingDebugPanel(UGameInstance* GameInstance) : Recorder(MakeUnique<FGASC_TargetingDebugRecorder>(GameInstance)) {}
FGASC_TargetingDebugPanel::~FGASC_TargetingDebugPanel() = default;
void FGASC_TargetingDebugPanel::OnDebugPanelClosed()
{
	bPanelWasOpen = false;
	PauseError.Reset();
	Recorder->ClearGhostMeshes();
}

void FGASC_TargetingDebugPanel::SetGamePaused(bool bPaused)
{
	Recorder->RefreshWorld();
	UWorld* World = Recorder->World.Get();
	if (World && (World->IsPaused() == bPaused || UGameplayStatics::SetGamePaused(World, bPaused)))
	{
		PauseError.Reset();
	}
	else
	{
		PauseError = bPaused ? TEXT("Pause request failed. A local player and a pausable game mode are required.") : TEXT("Resume request failed or was refused by the game mode.");
	}
}

void FGASC_TargetingDebugPanel::DrawDebugPanel(bool& bOpen)
{
	using namespace GASCTargetingHistory;
	auto& R = *Recorder;
	// Pause only on the closed-to-open transition, so Resume remains effective while inspecting history.
	if (!bPanelWasOpen) { bPanelWasOpen = true; SetGamePaused(true); }
	const double PanelStarted = FPlatformTime::Seconds();
	ON_SCOPE_EXIT { R.PanelMs = (FPlatformTime::Seconds() - PanelStarted) * 1000; R.PeakPanelMs = FMath::Max(R.PeakPanelMs, R.PanelMs); };
	if (!ImGui::Begin("Targeting History", &bOpen)) { if (!bOpen) OnDebugPanelClosed(); else Recorder->ClearGhostMeshes(); ImGui::End(); return; }
	R.LinesLeft = R.LineBudget; R.TargetsLeft = R.MaxDrawTargets; R.SkippedDraws = 0;
	R.RefreshWorld();
	GASC_DrawRewindWorldControls(R.World.Get(), PauseError);
	ImGui::Separator();
	ImGui::Checkbox("Record all presets", &R.bRecording); ImGui::SameLine(); ImGui::Checkbox("Follow latest", &R.bFollow);
	ImGui::SameLine();
	if (GASC_BeginRewindSettings())
	{
		ImGui::Combo("Capture detail (new records)", &R.CaptureDetail, "Bounds only\0Full actors\0");
		ImGui::SliderFloat("Capture CPU budget / frame (ms)", &R.CaptureBudgetMs, .1f, 5.f, "%.1f");
		ImGui::SliderInt("World line budget", &R.LineBudget, 64, 2048);
		ImGui::SliderInt("Max visible target ghosts", &R.MaxDrawTargets, 1, 256);
		ImGui::Checkbox("Historical actor bounds", &R.bGhosts);
		ImGui::Checkbox("World details / actor labels", &R.bWorldLabels);
		ImGui::Checkbox("Full actor ghosts", &R.bShowMeshGhosts);
		if (!R.GhostBaseMaterial.IsValid()) ImGui::TextWrapped("Ghost material unavailable: retain /Game/GASCourse/Game/Systems/Debugging/M_RewindGhost when cooking. Bounds and labels remain available.");
		ImGui::Text("Capture %.3f ms (peak %.3f) | panel %.3f ms (peak %.3f)", R.CaptureFrame == GFrameCounter ? R.CaptureFrameMs : R.LastCaptureMs, R.PeakCaptureMs, R.PanelMs, R.PeakPanelMs);
		ImGui::Text("CPU-budget drops: %d | snapshot data: %.1f MiB", R.BudgetDrops, double(R.HistoryBytes) / (1024 * 1024));
		ImGui::TextWrapped("Opaque gold ghosts include filtered candidates. The pool allows eight components per visible actor, creating at most two per update. Capture limits may omit data; one Blueprint call and GPU time cannot be bounded by the capture timer.");
		ImGui::EndPopup();
	}
	if (ImGui::Button("Clear history")) { R.History.Empty(); R.HistoryBytes = 0; R.Pending.Empty(); R.SelectedId = 0; R.Dropped = 0; R.BudgetDrops = 0; }
	ImGui::SameLine(); ImGui::Text("%d captures | %d in flight | %d evicted/dropped", R.History.Num(), R.Pending.Num(), R.Dropped);
	if (ImGui::CollapsingHeader("Filter recorded presets", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (!R.bPresetsLoaded) R.RefreshPresets();
		if (ImGui::Button("Refresh presets")) R.RefreshPresets();
		ImGui::SameLine(); if (ImGui::Button("Show all")) R.ViewPresets.Empty();
		ImGui::InputText("Search presets", R.Search, UE_ARRAY_COUNT(R.Search));
		ImGui::TextWrapped("All presets record automatically. Select presets to filter the timeline; no selection shows all recorded evaluations.");
		ImGui::BeginChild("Preset list", ImVec2(0, 150), true);
		for (const FString& Path : R.AvailablePresets)
		{
			if (!Path.Contains(UTF8_TO_TCHAR(R.Search))) continue;
			bool Selected = R.ViewPresets.Contains(Path);
			if (ImGui::Checkbox(TCHAR_TO_UTF8(*Path), &Selected)) { if (Selected) R.ViewPresets.Add(Path); else R.ViewPresets.Remove(Path); }
		}
		ImGui::EndChild();
	}
	TArray<int32> Visible;
	for (int32 I = 0; I < R.History.Num(); ++I) if (R.ViewPresets.IsEmpty() || R.ViewPresets.Contains(R.History[I].Preset)) Visible.Add(I);
	if (!Visible.IsEmpty())
	{
		if (R.bFollow) R.SelectedId = R.History[Visible.Last()].Id;
		int32 Index = Visible.IndexOfByPredicate([&](int32 I) { return R.History[I].Id == R.SelectedId; });
		if (Index == INDEX_NONE) Index = 0;
		uint64 Frame = R.History[Visible[Index]].Frame, MinFrame = R.History[Visible[0]].Frame, MaxFrame = R.History[Visible.Last()].Frame;
		if (ImGui::SliderScalar("Frame", ImGuiDataType_U64, &Frame, &MinFrame, &MaxFrame))
		{
			Index = 0; for (int32 I = 0; I < Visible.Num(); ++I) if (R.History[Visible[I]].Frame <= Frame) Index = I;
			R.bFollow = false;
		}
		if (ImGui::SliderInt("Evaluation timeline", &Index, 0, Visible.Num() - 1)) R.bFollow = false;
		ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * .6f));
		if (ImGui::Button("Previous") && Index > 0) { --Index; R.bFollow = false; } ImGui::SameLine();
		if (ImGui::Button("Next") && Index + 1 < Visible.Num()) { ++Index; R.bFollow = false; }
		if (ImGui::BeginCombo("Executions", TCHAR_TO_UTF8(*FString::Printf(TEXT("Frame %llu | %s"), R.History[Visible[Index]].Frame, *FPaths::GetBaseFilename(R.History[Visible[Index]].Preset)))))
		{
			for (int32 I = 0; I < Visible.Num(); ++I)
			{
				const auto& Item = R.History[Visible[I]]; ImGui::PushID(I);
				if (ImGui::Selectable(TCHAR_TO_UTF8(*FString::Printf(TEXT("Frame %llu | %s"), Item.Frame, *Item.Preset)), I == Index)) { Index = I; R.bFollow = false; }
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		FEvaluation& E = R.History[Visible[Index]]; R.SelectedId = E.Id;
		ImGui::TextWrapped("%s\nSource: %s | Frame %llu -> %llu | %.3fs | %s", TCHAR_TO_UTF8(*E.Preset), TCHAR_TO_UTF8(*E.Source), E.Frame, E.EndFrame, E.Time, TCHAR_TO_UTF8(*E.Status));
		if (E.bTruncated) ImGui::TextColored(ImVec4(1, .5f, 0, 1), "TRUNCATED: capture limits reached; this is not a complete evaluation.");
		TFunction<void(int32)> DrawStageUI;
		DrawStageUI = [&](int32 I)
		{
			auto& S = E.Stages[I]; ImGui::PushID(I);
			ImGui::Checkbox("World", &S.bDraw); ImGui::SameLine();
			if (ImGui::TreeNode("Stage", "%s%s (frame %llu -> %llu)", S.ParentStage != INDEX_NONE ? "Child: " : "", TCHAR_TO_UTF8(*S.Name), S.Frame, S.EndFrame))
			{
				if (S.bCondition)
				{
					ImGui::TextWrapped("%s", TCHAR_TO_UTF8(*S.Summary));
					ImGui::TextUnformatted("Configured Children (in order):"); ImGui::TextUnformatted(TCHAR_TO_UTF8(*S.ChildrenDescription));
				}
				else Table(S.Rows, false, S.bFilter, S.bScoring);
				for (int32 J = I + 1; J < E.Stages.Num(); ++J) if (E.Stages[J].ParentStage == I) DrawStageUI(J);
				ImGui::TreePop();
			}
			ImGui::PopID();
		};
		for (int32 I = 0; I < E.Stages.Num(); ++I) if (E.Stages[I].ParentStage == INDEX_NONE) DrawStageUI(I);
		struct FWorldRow { const FTarget* Target; FString Details; FColor Color; };
		TArray<FWorldRow> WorldRows;
		bool AnyWorldStage = false;
		for (int32 I = 0; I < E.Stages.Num(); ++I)
		{
			FStage& S = E.Stages[I];
			if (S.bDraw && R.World.IsValid())
			{
				AnyWorldStage = true;
				R.DrawDistances(S);
				for (const auto& P : S.Primitives) R.DrawShape(P);
				for (const auto& T : S.Rows)
				{
					const FString Result = !S.bScoring ? FString() : T.bUnknown ? TEXT("Comparison unavailable") : FString::Printf(TEXT("Before %.4f | Added %+.4f | Total %s"), T.Before, T.Added, T.bRemoved ? TEXT("--") : *FString::Printf(TEXT("%.4f"), T.Result.Score));
					const FString Details = S.Name + TEXT("\n") + Outcome(T, S.bFilter) + TEXT(" | ") + Result + TEXT("\n") + T.Info;
					// Combine processor labels for the same actor so they do not overprint each other.
					if (auto* Existing = WorldRows.FindByPredicate([&](const FWorldRow& Row) { return Row.Target->Name == T.Name && Row.Target->Transform.Equals(T.Transform); }))
					{
						if (Existing->Details.Len() < 2048) Existing->Details += TEXT("\n") + Details;
						if (T.bRemoved) Existing->Color = FColor::Red;
					}
					else if (WorldRows.Num() < R.MaxDrawTargets) WorldRows.Add({ &T, Details, T.bRemoved ? FColor::Red : FColor::Cyan });
					else ++R.SkippedDraws;
				}
			}
		}
		if (ImGui::CollapsingHeader("Final targets (highest score first)", ImGuiTreeNodeFlags_DefaultOpen)) Table(E.Final, true);
		for (const auto& Row : WorldRows) R.DrawTarget(*Row.Target, Row.Color, Row.Details);
		if (R.World.IsValid())
		{
			int32 Labels = 0;
			for (const auto* T : ProcessedActors(E))
			{
				if (++Labels > R.MaxDrawTargets) { ++R.SkippedDraws; continue; }
				if (R.bGhosts && !AnyWorldStage) { TGuardValue<bool> HideDuplicateLabels(R.bWorldLabels, false); R.DrawTarget(*T, T->bRemoved ? FColor::Red : FColor::Silver); }
				const bool Winner = !E.Final.IsEmpty() && T->Result.Score == E.Final[0].Result.Score && !T->bRemoved;
				DrawDebugString(R.World.Get(), T->Transform.TransformPosition(T->LocalCenter) + FVector(0, 0, T->Extent.Size() + 10), T->Name + TEXT("\n") + FinalLabel(E, *T), nullptr, Winner ? FColor::Green : FColor::White, 0, true);
			}
		}
		R.ShowGhostMeshes(E);
		ImGui::Text("World budget: %d / %d line units; %d omitted overlays", R.LineBudget - R.LinesLeft, R.LineBudget, R.SkippedDraws);
	}
	else R.ClearGhostMeshes();
	if (!bOpen) OnDebugPanelClosed();
	ImGui::End();
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Game/Systems/Targeting/Sort/GASCourse_TargetSortDistance.h"
#include "Game/Systems/Targeting/AreaofEffect/GASC_TargetFilter_ActorClass.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_TargetingHistoryTest, "GASCourse.Targeting.History.EvaluationAndLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_TargetingHistoryTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), TestWorld)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(TestWorld);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(TestWorld); TestWorld->DestroyWorld(false); };
	FGASC_TargetingDebugRecorder R(nullptr); R.World = TestWorld; R.bRecording = true; R.CaptureBudgetMs = 1000.f;
	auto* TestInstance = NewObject<UGameInstance>(GEngine);
	TestWorld->SetGameInstance(TestInstance);
	auto* System = NewObject<UTargetingSubsystem>(TestInstance);
	if (!TestNotNull(TEXT("Targeting system"), System)) return false;
	APawn* Source = TestWorld->SpawnActor<APawn>(); AActor* Target = TestWorld->SpawnActor<AActor>();
	for (AActor* Actor : {static_cast<AActor*>(Source), Target}) { auto* Root = NewObject<USceneComponent>(Actor); Actor->SetRootComponent(Root); Root->RegisterComponent(); }
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (AActor* Actor : {static_cast<AActor*>(Source), Target})
	{
		auto* Mesh = NewObject<UStaticMeshComponent>(Actor); Actor->AddInstanceComponent(Mesh);
		Mesh->SetupAttachment(Actor->GetRootComponent()); Mesh->SetStaticMesh(Cube); Mesh->RegisterComponent();
	}
	Target->SetActorLocation(FVector(200, 0, 0));
	auto* Preset = NewObject<UTargetingPreset>();
	UClass* GatherClass = FindObject<UClass>(nullptr, TEXT("/Script/TargetingSystem.TargetingSelectionTask_SourceActor"));
	if (!TestNotNull(TEXT("Native gather processor"), GatherClass)) return false;
	Preset->TargetingTaskSet.Tasks.Add(NewObject<UTargetingTask>(Preset, GatherClass));
	auto* Positive = NewObject<UGASCourse_TargetSortDistance>(Preset); Positive->MaxDistance = 1000; Positive->DefaultScoreMultiplier = 1;
	auto* Negative = NewObject<UGASCourse_TargetSortDistance>(Preset); Negative->MaxDistance = 1000; Negative->DefaultScoreMultiplier = -.5f;
	Preset->TargetingTaskSet.Tasks.Add(Positive); Preset->TargetingTaskSet.Tasks.Add(Negative);
	R.ViewPresets.Add(TEXT("Unrelated view filter must not restrict capture"));
	IConsoleVariable* DisableDistance = IConsoleManager::Get().FindConsoleVariable(TEXT("GASCourseDebug.Targeting.Disable.Sort.Distance"));
	const int32 SavedDisabled = DisableDistance ? DisableDistance->GetInt() : 0;
	if (DisableDistance) DisableDistance->Set(0, ECVF_SetByCode);
	ON_SCOPE_EXIT { if (DisableDistance) DisableDistance->Set(SavedDisabled, ECVF_SetByCode); };
	FTargetingSourceContext SourceContext; SourceContext.SourceActor = Source;
	TArray<FTargetingRequestHandle> TestHandles;
	ON_SCOPE_EXIT { for (auto H : TestHandles) if (FTargetingSourceContext::Find(H)) System->RemoveAsyncTargetingRequestWithHandle(H); };
	auto NewHandle = [&]()
	{
		auto H = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext);
		TestHandles.Add(H);
		FTargetingDefaultResultData Seed; Seed.HitResult = FHitResult(Target, nullptr, Target->GetActorLocation(), FVector::UpVector); Seed.Score = .25f;
		FTargetingDefaultResultsSet::FindOrAdd(H).TargetResults.Add(Seed);
		return H;
	};
	auto Handle = NewHandle();
	System->ExecuteTargetingRequestWithHandle(Handle);
	if (!TestEqual(TEXT("Immediate request captured once"), R.History.Num(), 1)) return false;
	const auto& E = R.History[0];
	if (!TestEqual(TEXT("Each processor captured once"), E.Stages.Num(), 3)) return false;
	TestEqual(TEXT("Gather records acquisition"), E.Stages[0].Rows.FilterByPredicate([](const auto& T) { return T.bAcquired; }).Num(), 1);
	TestTrue(TEXT("Positive contributions descending"), FMath::IsNearlyEqual(E.Stages[1].Rows[0].Added, 1.f));
	TestTrue(TEXT("Negative contributions descending"), FMath::IsNearlyEqual(E.Stages[2].Rows[0].Added, -.4f, 1.e-4f));
	TestTrue(TEXT("Before reflects previous processor total"), FMath::IsNearlyEqual(E.Stages[2].Rows[0].Before, 1.05f));
	TestTrue(TEXT("Final score is additive"), FMath::IsNearlyEqual(E.Final[0].Result.Score, .65f));
	TestEqual(TEXT("Final winner differs from highest first contribution"), E.Final[0].Name, Target->GetName());
	TestTrue(TEXT("Distance radius was recorded"), E.Stages[1].Primitives.Num() == 1 && E.Stages[1].Primitives[0].Radius == 1000.f);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
	TestEqual(TEXT("Completed history survives handle release"), R.History.Num(), 1);
	Target->SetActorLocation(FVector(400, 0, 0));
	TestTrue(TEXT("Historical transform does not follow actor"), R.History[0].Final[0].Transform.GetLocation().Equals(FVector(200, 0, 0)));
	Target->SetActorLocation(FVector(200, 0, 0));

	auto Async = NewHandle(); System->StartAsyncTargetingRequestWithHandle(Async);
	static_cast<FTickableGameObject*>(System)->Tick(.016f);
	if (!TestEqual(TEXT("Async request captured once"), R.History.Num(), 2)) return false;
	TestEqual(TEXT("Async processors are not duplicated"), R.History.Last().Stages.Num(), 3);
	TestTrue(TEXT("Async total matches immediate total"), FMath::IsNearlyEqual(R.History.Last().Final[0].Result.Score, .65f));
	UTargetingSubsystem::ReleaseTargetRequestHandle(Async);
	auto Cancel = NewHandle(); System->StartAsyncTargetingRequestWithHandle(Cancel); System->RemoveAsyncTargetingRequestWithHandle(Cancel);
	TestEqual(TEXT("Cancellation closes pending capture"), R.Pending.Num(), 0);
	TestEqual(TEXT("Cancellation status"), R.History.Last().Status, FString(TEXT("Cancelled")));

	auto* Filter = NewObject<UGASC_TargetFilter_ActorClass>(Preset); Filter->AddIgnoredActorClassFilter(AActor::StaticClass()); Preset->TargetingTaskSet.Tasks.Add(Filter);
	Handle = NewHandle(); System->ExecuteTargetingRequestWithHandle(Handle);
	TestTrue(TEXT("All filtered targets produce an empty final set"), R.History.Last().Final.IsEmpty());
	TestEqual(TEXT("Both removed targets retained in stage history"), R.History.Last().Stages.Last().Rows.Num(), 2);
	TestEqual(TEXT("Ghost candidates include actors removed by filters"), GASCTargetingHistory::ProcessedActors(R.History.Last()).Num(), 2);
	TestTrue(TEXT("Removed target label distinguishes unavailable final score from last score"), GASCTargetingHistory::FinalLabel(R.History.Last(), R.History.Last().Stages.Last().Rows[0]).Contains(TEXT("final score: --")));
	for (const auto& Row : R.History.Last().Stages.Last().Rows) { TestTrue(TEXT("Removed status"), Row.bRemoved); TestTrue(TEXT("Reason captured"), Row.Info.Contains(TEXT("ignored class"))); }
	UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
	Target->Destroy();
	TestTrue(TEXT("Destroyed actor's historical location remains"), R.History[0].Final[0].Transform.GetLocation().Equals(FVector(200, 0, 0)));
	R.bRecording = false; Handle = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext); System->ExecuteTargetingRequestWithHandle(Handle); UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
	TestEqual(TEXT("Stopping capture prevents new evaluations"), R.History.Num(), 4);
	R.bRecording = true;
	Preset->TargetingTaskSet.Tasks.RemoveAt(3);
	auto Requeue = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext); TestHandles.Add(Requeue);
	System->StartAsyncTargetingRequestWithHandle(Requeue);
	FTargetingAsyncTaskData::FindOrAdd(Requeue).bRequeueOnCompletion = true;
	static_cast<FTickableGameObject*>(System)->Tick(.016f);
	static_cast<FTickableGameObject*>(System)->Tick(.016f);
	TestEqual(TEXT("Requeues create independent evaluations"), R.History.Num(), 6);
	TestEqual(TEXT("Next requeue is in flight"), R.Pending.Num(), 1);
	System->RemoveAsyncTargetingRequestWithHandle(Requeue);
	TestEqual(TEXT("Cancelling a requeue closes capture"), R.Pending.Num(), 0);
	auto Partial = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext); TestHandles.Add(Partial);
	R.OnEvent(Partial, nullptr, ETargetingEvaluationDebugEvent::BeginRequest);
	FTargetingDefaultResultData Incoming; Incoming.HitResult = FHitResult(Source, nullptr, Source->GetActorLocation(), FVector::UpVector);
	FTargetingDefaultResultsSet::FindOrAdd(Partial).TargetResults.Add(Incoming);
	R.OnEvent(Partial, Positive, ETargetingEvaluationDebugEvent::BeginTask);
	R.OnEvent(Partial, nullptr, ETargetingEvaluationDebugEvent::CancelRequest);
	TestEqual(TEXT("Incomplete stage retains incoming targets"), R.History.Last().Stages.Last().Rows.Num(), 1);
	TestTrue(TEXT("Incomplete stage does not fabricate outcome"), R.History.Last().Stages.Last().Rows[0].bUnknown);
	auto DelayedFinish = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext); TestHandles.Add(DelayedFinish);
	R.OnEvent(DelayedFinish, nullptr, ETargetingEvaluationDebugEvent::BeginRequest);
	FTargetingDefaultResultsSet::FindOrAdd(DelayedFinish).TargetResults.Add(Incoming);
	R.OnEvent(DelayedFinish, Positive, ETargetingEvaluationDebugEvent::BeginTask);
	R.OnEvent(DelayedFinish, Positive, ETargetingEvaluationDebugEvent::EndTask);
	Source->SetActorLocation(FVector(100, 0, 0));
	R.OnEvent(DelayedFinish, nullptr, ETargetingEvaluationDebugEvent::EndRequest);
	TestTrue(TEXT("Final ghosts use task completion pose, not later subsystem tick pose"), R.History.Last().Final[0].Transform.GetLocation().IsNearlyZero());
	TestEqual(TEXT("Full actor capture is the default"), R.CaptureDetail, 1);
	TestEqual(TEXT("Default captures contain target meshes"), R.History[0].Final[0].Meshes.Num(), 1);
	TestEqual(TEXT("Source mesh captured without enabling a processor"), R.History[0].Stages[0].SourceGhost.Meshes.Num(), 1);
	R.LinesLeft = 512; R.TargetsLeft = 12; R.SkippedDraws = 0; R.bWorldLabels = false;
	const int32 LinesBefore = TestWorld->GetLineBatcher(UWorld::ELineBatcherType::World)->BatchedLines.Num();
	for (int32 I = 0; I < 200; ++I) R.DrawTarget(R.History[0].Final[0], FColor::Cyan);
	TestEqual(TEXT("Draw target cap skips excess ghosts"), R.SkippedDraws, 188);
	TestTrue(TEXT("Actual submitted debug lines stay below the line budget"), TestWorld->GetLineBatcher(UWorld::ELineBatcherType::World)->BatchedLines.Num() - LinesBefore <= 512);
	R.bShowMeshGhosts = true; R.bFollow = true; R.ShowGhostMeshes(R.History.Last());
	TestTrue(TEXT("Live follow supports pooled actor ghosts"), R.GhostActor.IsValid());
	TestTrue(TEXT("Ghost material is opaque"), R.GhostMaterial.IsValid() && R.GhostMaterial->GetBlendMode() == BLEND_Opaque);
	FLinearColor Tint;
	TestTrue(TEXT("Ghost material exposes gold color parameter"), R.GhostMaterial.IsValid() && R.GhostMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), Tint) && Tint.Equals(FLinearColor(.38f, .23f, .025f, 1.f)));
	const auto GhostActorBefore = R.GhostActor;
	const int32 PoolBefore = R.StaticPool.Num();
	R.ShowGhostMeshes(R.History.Last());
	TestTrue(TEXT("Repeated playback reuses ghost actor"), GhostActorBefore == R.GhostActor);
	TestEqual(TEXT("Repeated playback reuses mesh components"), R.StaticPool.Num(), PoolBefore);
	const int32 CapturesBeforeBudget = R.History.Num();
	R.CaptureBudgetMs = 0;
	auto BudgetHandle = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext); TestHandles.Add(BudgetHandle);
	System->ExecuteTargetingRequestWithHandle(BudgetHandle);
	TestEqual(TEXT("Exhausted CPU budget does not start another capture"), R.History.Num(), CapturesBeforeBudget);
	TestTrue(TEXT("Budget omissions are visible"), R.BudgetDrops > 0);
	AddInfo(FString::Printf(TEXT("Synthetic targeting capture CPU: %.3f ms total for this test frame; full scene/GPU timing is not measured by NullRHI."), R.CaptureFrameMs));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_TargetingHistoryRetentionTest, "GASCourse.Targeting.History.RetentionAndOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_TargetingHistoryRetentionTest::RunTest(const FString& Parameters)
{
	FGASC_TargetingDebugRecorder R(nullptr);
	for (uint32 I = 0; I < 140; ++I) { auto& E = R.Pending.FindOrAdd(I); E.Id = I + 1; E.Frame = 140 - I; R.Commit(I); }
	TestEqual(TEXT("History has a bounded number of requests"), R.History.Num(), GASCTargetingHistory::MaxRecords);
	TestEqual(TEXT("Evicted capture count"), R.Dropped, 12);
	for (int32 I = 1; I < R.History.Num(); ++I) TestTrue(TEXT("Timeline is chronological even with out-of-order completion"), R.History[I - 1].Frame <= R.History[I].Frame);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_TargetingTagQueryHistoryTest, "GASCourse.Targeting.History.TagQueryDetails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_TargetingTagQueryHistoryTest::RunTest(const FString& Parameters)
{
	using namespace GASCTargetingHistory;
	UClass* FilterClass = LoadClass<UTargetingTask>(nullptr, TEXT("/Game/GASCourse/Game/Character/Player/Targeting/Filters/BP_TargetFilter_MatchesTagQuery.BP_TargetFilter_MatchesTagQuery_C"));
	if (!TestNotNull(TEXT("Matches-tag-query Blueprint"), FilterClass)) return false;
	auto* Task = NewObject<UTargetingTask>(GetTransientPackage(), FilterClass);
	auto* Property = FindFProperty<FStructProperty>(FilterClass, TEXT("TagQuery"));
	if (!TestNotNull(TEXT("Blueprint query property"), Property)) return false;
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Input.NativeAction.Move"));
	FGameplayTagQueryExpression Expr; Expr.AllTagsMatch().AddTag(Tag);
	*Property->ContainerPtrToValuePtr<FGameplayTagQuery>(Task) = FGameplayTagQuery::BuildQuery(Expr);
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(TestWorld);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(TestWorld); TestWorld->DestroyWorld(false); };
	AActor* Actor = TestWorld->SpawnActor<AActor>();
	auto* ASC = NewObject<UAbilitySystemComponent>(Actor); Actor->AddInstanceComponent(ASC); ASC->RegisterComponent();
	FTargetingDefaultResultData Result; Result.HitResult = FHitResult(Actor, nullptr, FVector::ZeroVector, FVector::UpVector);
	FTargetingRequestHandle Handle = UTargetingSubsystem::CreateTargetRequestHandle();
	ON_SCOPE_EXIT { UTargetingSubsystem::ReleaseTargetRequestHandle(Handle); };
	auto Evaluate = [&]()
	{
		auto& Results = FTargetingDefaultResultsSet::FindOrAdd(Handle).TargetResults; Results.Reset(); Results.Add(Result);
		Task->Execute(Handle);
		const bool Filtered = Results.IsEmpty();
		return TPair<bool, FString>(Filtered, TargetInfo(Task, Handle, Result, Filtered));
	};
	const auto Absent = Evaluate();
	TestTrue(TEXT("Absent-tag reason includes query expression and missing tag"), Absent.Value.Contains(TEXT("AllTagsMatch")) && Absent.Value.Contains(TEXT("Input.NativeAction.Move=absent")) && Absent.Value.Contains(TEXT("query did not match")));
	ASC->AddLooseGameplayTag(Tag);
	const auto Present = Evaluate();
	TestTrue(TEXT("Present-tag reason explains match"), Present.Value.Contains(TEXT("Input.NativeAction.Move=present")) && Present.Value.Contains(TEXT("query matched")));
	TestTrue(TEXT("Actual Blueprint changes filter decision when query match changes"), Absent.Key != Present.Key);
	FTarget Row; TestEqual(TEXT("Filter survivor label"), FString(Outcome(Row, true)), FString(TEXT("Not Filtered")));
	Row.bRemoved = true; TestEqual(TEXT("Removed label"), FString(Outcome(Row, true)), FString(TEXT("Filtered")));
	AddInfo(FString::Printf(TEXT("Blueprint query policy: absent=%s; present=%s"), Absent.Key ? TEXT("Filtered") : TEXT("Not Filtered"), Present.Key ? TEXT("Filtered") : TEXT("Not Filtered")));
	return true;
}

#if WITH_EDITOR
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderingThread.h"
#include "Misc/App.h"
#include "Misc/DataValidation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_TargetingGhostRenderTest, "GASCourse.Targeting.History.SkeletalGhostRendering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGASC_TargetingGhostRenderTest::RunTest(const FString& Parameters)
{
	using namespace GASCTargetingHistory;
	if (!FApp::CanEverRender()) { AddInfo(TEXT("Render test requires a real RHI; skipped under NullRHI.")); return true; }
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(TestWorld);
	ON_SCOPE_EXIT { FlushRenderingCommands(); GEngine->DestroyWorldContext(TestWorld); TestWorld->DestroyWorld(false); };
	auto* Asset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Sword_Animations/Demo/Mannequin_UE4/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (!TestNotNull(TEXT("Skeletal render fixture"), Asset)) return false;
	FGASC_TargetingDebugRecorder R(nullptr); R.World = TestWorld;
	TestTrue(TEXT("Ghost base has pre-authored skeletal and clothing shader support"), R.GhostBaseMaterial.IsValid() && R.GhostBaseMaterial->GetMaterial()->GetUsageByFlag(MATUSAGE_SkeletalMesh) && R.GhostBaseMaterial->GetMaterial()->GetUsageByFlag(MATUSAGE_Clothing));
	FTarget Target; Target.Name = TEXT("Skeletal render fixture");
	auto Pose = MakeShared<GASCTargetingHistory::FMeshPose>(); Pose->SkeletalMesh.Reset(Asset); Pose->Transform = FTransform::Identity;
	Pose->LocalBones = Asset->GetRefSkeleton().GetRefBonePose(); Target.Meshes.Add(Pose);
	FEvaluation E; E.Id = 1; E.Final.Add(Target);
	auto* CaptureActor = TestWorld->SpawnActor<AActor>();
	auto* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor); CaptureActor->AddInstanceComponent(Capture);
	Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
	auto* Texture = NewObject<UTextureRenderTarget2D>(Capture); Texture->InitAutoFormat(128, 128); Capture->TextureTarget = Texture;
	Capture->SetWorldLocationAndRotation(FVector(350, 0, 100), FRotator(0, 180, 0)); Capture->RegisterComponent();
	for (int32 I = 0; I < 6; ++I)
	{
		E.Id = I + 1; Pose->Transform.SetLocation(FVector(0, I * 5, 0));
		R.ShowGhostMeshes(E);
		if (!TestEqual(TEXT("One pooled skeletal component"), R.SkeletalPool.Num(), 1)) return false;
		TestTrue(TEXT("Ghost has its historical pose"), R.SkeletalPool[0]->BoneSpaceTransforms.Num() == Pose->LocalBones.Num());
		Capture->CaptureScene(); FlushRenderingCommands();
		if (I == 2) R.ClearGhostMeshes();
	}
	R.ClearGhostMeshes(); Capture->CaptureScene(); FlushRenderingCommands();
	AddInfo(TEXT("Rendered six skeletal ghost updates and pool teardown/recreation using the active RHI."));
	return true;
}
#endif

#include "Game/Systems/Targeting/Conditional/GASC_TargetingCondition_MatchTagQuery.h"
#include "Game/Systems/Targeting/Conditional/GASC_TargetingPreset_BlockingExample.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGASC_TargetingConditionalHistoryTest, "GASCourse.Targeting.History.ConditionalChildren",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGASC_TargetingConditionalHistoryTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(TestWorld);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(TestWorld); TestWorld->DestroyWorld(false); };
	auto* Instance = NewObject<UGameInstance>(GEngine); TestWorld->SetGameInstance(Instance);
	auto* System = NewObject<UTargetingSubsystem>(Instance);
	FGASC_TargetingDebugRecorder R(nullptr); R.World = TestWorld; R.CaptureBudgetMs = 1000;
	auto* Actor = TestWorld->SpawnActor<APawn>();
	auto* Root = NewObject<USceneComponent>(Actor); Actor->SetRootComponent(Root); Root->RegisterComponent();
	auto* ASC = NewObject<UAbilitySystemComponent>(Actor); Actor->AddInstanceComponent(ASC); ASC->RegisterComponent();
	const auto Tag = FGameplayTag::RequestGameplayTag(TEXT("Input.NativeAction.Move"));
	auto* Preset = NewObject<UTargetingPreset>();
	auto* GatherClass = FindObject<UClass>(nullptr, TEXT("/Script/TargetingSystem.TargetingSelectionTask_SourceActor"));
	Preset->TargetingTaskSet.Tasks.Add(NewObject<UTargetingTask>(Preset, GatherClass));
	auto* Condition = NewObject<UGASC_TargetingCondition_MatchTagQuery>(Preset);
	FGameplayTagQueryExpression Match; Match.AllTagsMatch().AddTag(Tag); Condition->MatchTagQuery = FGameplayTagQuery::BuildQuery(Match);
	auto* Score = NewObject<UGASCourse_TargetSortDistance>(Condition); Score->MaxDistance = 1000; Score->DefaultScoreMultiplier = 1;
	Condition->Children.Add(Score);
	auto* Nested = NewObject<UGASC_TargetingCondition_MatchTagQuery>(Condition);
	FGameplayTagQueryExpression NoMatch; NoMatch.NoTagsMatch().AddTag(Tag); Nested->MatchTagQuery = FGameplayTagQuery::BuildQuery(NoMatch);
	auto* SkippedScore = NewObject<UGASCourse_TargetSortDistance>(Nested); SkippedScore->DefaultScoreMultiplier = 100;
	Nested->Children.Add(SkippedScore); Condition->Children.Add(Nested); Preset->TargetingTaskSet.Tasks.Add(Condition);
	auto* Tail = NewObject<UGASCourse_TargetSortDistance>(Preset); Tail->DefaultScoreMultiplier = -.5f; Preset->TargetingTaskSet.Tasks.Add(Tail);
	FTargetingSourceContext Context; Context.SourceActor = Actor; Context.InstigatorActor = Actor;
	auto Run = [&](bool Async)
	{
		auto H = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Context);
		if (Async) { System->StartAsyncTargetingRequestWithHandle(H); static_cast<FTickableGameObject*>(System)->Tick(.016f); }
		else System->ExecuteTargetingRequestWithHandle(H);
		UTargetingSubsystem::ReleaseTargetRequestHandle(H);
	};
	ASC->AddLooseGameplayTag(Tag); Run(false);
	if (!TestEqual(TEXT("Matching condition records parent, child and nested condition"), R.History.Last().Stages.Num(), 5)) return false;
	TestTrue(TEXT("Children are explicitly nested in history"), R.History.Last().Stages[2].ParentStage == 1 && R.History.Last().Stages[3].ParentStage == 1);
	TestTrue(TEXT("Conditional additions preserve later sibling scoring"), FMath::IsNearlyEqual(R.History.Last().Final[0].Result.Score, .5f));
	TestTrue(TEXT("Failed nested condition explains skipped children"), R.History.Last().Stages[3].Summary.Contains(TEXT("skip Children")));
	Run(true); TestTrue(TEXT("Async children have matching additive result"), FMath::IsNearlyEqual(R.History.Last().Final[0].Result.Score, .5f));
	ASC->RemoveLooseGameplayTag(Tag); Run(false);
	TestEqual(TEXT("Unmatched branch skips all descendants but runs later siblings"), R.History.Last().Stages.Num(), 3);
	TestTrue(TEXT("Skipped children do not contribute"), FMath::IsNearlyEqual(R.History.Last().Final[0].Result.Score, -.5f));
	Run(true); TestEqual(TEXT("Async skipped branch still completes"), R.History.Last().Stages.Num(), 3);
	TestEqual(TEXT("Preset root task list is never flattened in place"), Preset->TargetingTaskSet.Tasks.Num(), 3);
	ASC->AddLooseGameplayTag(Tag);
	auto Requeue = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Context);
	System->StartAsyncTargetingRequestWithHandle(Requeue); FTargetingAsyncTaskData::FindOrAdd(Requeue).bRequeueOnCompletion = true;
	static_cast<FTickableGameObject*>(System)->Tick(.016f); ASC->RemoveLooseGameplayTag(Tag);
	static_cast<FTickableGameObject*>(System)->Tick(.016f);
	TestTrue(TEXT("Requeue reevaluates query instead of reusing old branch decision"), FMath::IsNearlyEqual(R.History.Last().Final[0].Result.Score, -.5f));
	System->RemoveAsyncTargetingRequestWithHandle(Requeue);
	TestEqual(TEXT("Cancelling conditional request closes pending history"), R.Pending.Num(), 0);
	ASC->AddLooseGameplayTag(Tag);
	auto* TraceClass = FindObject<UClass>(nullptr, TEXT("/Script/TargetingSystem.TargetingSelectionTask_Trace"));
	auto* Trace = NewObject<UTargetingTask>(Condition, TraceClass); Condition->Children.Insert(Trace, 0);
	auto Waiting = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Context);
	System->StartAsyncTargetingRequestWithHandle(Waiting); static_cast<FTickableGameObject*>(System)->Tick(.016f);
	const auto* WaitingState = FTargetingAsyncTaskData::Find(Waiting);
	TestTrue(TEXT("Real async trace child remains executing"), WaitingState && WaitingState->CurrentAsyncTaskState == ETargetingTaskAsyncState::Executing);
	TestTrue(TEXT("Async scheduler owns the child task, not its parent condition"), WaitingState && (*FTargetingTaskSet::Find(Waiting))->Tasks[WaitingState->CurrentAsyncTaskIndex] == Trace);
	System->RemoveAsyncTargetingRequestWithHandle(Waiting);
	TestEqual(TEXT("Cancelling an executing child closes its parent request"), R.History.Last().Status, FString(TEXT("Cancelled")));
	TestTrue(TEXT("Cancelled child remains nested in partial history"), R.History.Last().Stages.Last().ParentStage == 1 && !R.History.Last().Stages.Last().bCompleted);
	auto* Example = NewObject<UGASC_TargetingPreset_BlockingExample>();
	TestEqual(TEXT("Example preset is ready to edit"), Example->TargetingTaskSet.Tasks.Num(), 2);
	const auto* ExampleCondition = Cast<UGASC_TargetingCondition_MatchTagQuery>(Example->TargetingTaskSet.Tasks[1]);
	TestTrue(TEXT("Example has configured blocking query and two inline children"), ExampleCondition && !ExampleCondition->MatchTagQuery.IsEmpty() && ExampleCondition->Children.Num() == 2);
#if WITH_EDITOR
	FDataValidationContext ValidContext;
	TestTrue(TEXT("Designer example validates at preset level"), Example->IsDataValid(ValidContext) == EDataValidationResult::Valid);
	auto* EditableCondition = Cast<UGASC_TargetingCondition_MatchTagQuery>(Example->TargetingTaskSet.Tasks[1]);
	EditableCondition->MatchTagQuery = FGameplayTagQuery(); FDataValidationContext InvalidContext;
	TestTrue(TEXT("Preset validation catches empty child condition query"), Example->IsDataValid(InvalidContext) == EDataValidationResult::Invalid);
#endif
	return true;
}

#endif


#endif // !UE_BUILD_SHIPPING

