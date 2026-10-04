#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "MontageRetimingData.h"
#include "MontageRetimingAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    struct FRetimingFixture
    {
        TStrongObjectPtr<USkeleton> Skeleton{NewObject<USkeleton>()};
        TStrongObjectPtr<UAnimSequence> Sequence{NewObject<UAnimSequence>()};
        TStrongObjectPtr<UAnimMontage> Montage{NewObject<UAnimMontage>()};
        TStrongObjectPtr<USkeletalMeshComponent> Mesh{NewObject<USkeletalMeshComponent>()};
        TStrongObjectPtr<UMontageRetimingAnimInstance> Instance{NewObject<UMontageRetimingAnimInstance>(Mesh.Get())};
        UMontageRetimingSettings* Settings = nullptr;

        FRetimingFixture()
        {
            {
                FReferenceSkeletonModifier Modifier(Skeleton.Get());
                Modifier.Add(FMeshBoneInfo(TEXT("root"), TEXT("root"), INDEX_NONE), FTransform::Identity);
            }
            Sequence->SetSkeleton(Skeleton.Get());
            auto& Controller = Sequence->GetController();
            Controller.InitializeModel();
            Controller.SetFrameRate(FFrameRate(30, 1), false);
            Controller.SetNumberOfFrames(FFrameNumber(60), false);
            Sequence->WaitOnExistingCompression();
            Montage->SetSkeleton(Skeleton.Get());
            Montage->SlotAnimTracks.Reset();
            auto& Slot = Montage->AddSlot(TEXT("DefaultSlot"));
            FAnimSegment Segment;
            Segment.SetAnimReference(Sequence.Get());
            Segment.AnimEndTime = 2;
            Slot.AnimTrack.AnimSegments.Add(Segment);
            // Invoke the exported virtual through the base interface.
            static_cast<UAnimCompositeBase*>(Montage.Get())->SetCompositeLength(2);
            Montage->CompositeSections.Reset();
            Montage->AddAnimCompositeSection(TEXT("A"), 0);
            Montage->AddAnimCompositeSection(TEXT("B"), 1);
            Montage->CompositeSections[0].NextSectionName = TEXT("B");
            Montage->CompositeSections[1].NextSectionName = NAME_None;
            Montage->bEnableAutoBlendOut = false;
            Settings = NewObject<UMontageRetimingSettings>(Montage.Get());
            Settings->bEnabled = true;
            Montage->AddMetaData(Settings);
            Target(0, 2); // A: 30 source frames -> 60 target frames, 0.5x.
            Target(1, 0.5); // B: 30 source frames -> 15 target frames, 2x.
            Instance->CurrentSkeleton = Skeleton.Get();
            Mesh->AnimScriptInstance = Instance.Get();
        }
        void Target(int32 Index, double Seconds)
        {
            auto* Data = MontageRetiming::FindSection(*Montage, Index);
            if (!Data)
            {
                Data = NewObject<UMontageRetimingSection>(Montage.Get());
                Data->Identity = FGuid::NewGuid();
                Montage->CompositeSections[Index].MetaData.Add(Data);
            }
            Data->bOverride = true;
            Data->TargetSeconds = Seconds;
        }
        FAnimMontageInstance* Play(float Rate = 1)
        {
            Instance->Montage_Play(Montage.Get(), Rate, EMontagePlayReturnType::MontageLength, 0, false);
            return Instance->GetActiveInstanceForMontage(Montage.Get());
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingBoundaryTest, "DeckKnight.MontageRetiming.Playback.BoundaryAndExternalRate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingBoundaryTest::RunTest(const FString&)
{
    FRetimingFixture F;
    auto* MI = F.Play();
    if (!TestNotNull(TEXT("Ordinary Montage_Play creates an instance"), MI)) return false;
    F.Instance->Montage_Advance(2.125f);
    TestTrue(TEXT("One large tick crosses A at 0.5x then B at 2x"), FMath::IsNearlyEqual(MI->GetPosition(), 1.25f, 0.001f));
    TestEqual(TEXT("External rate is restored after advancement"), MI->GetPlayRate(), 1.f);
    TestTrue(TEXT("Movement record covers the entire tick"), FMath::IsNearlyEqual(MI->GetDeltaMoved(), 1.25f, 0.001f));
    TestEqual(TEXT("Section marker stays at authored position"), F.Montage->CompositeSections[1].GetTime(), 1.f);
    MI->SetPosition(0);
    F.Instance->Montage_SetPlayRate(F.Montage.Get(), 2);
    F.Instance->Montage_Advance(0.5f);
    TestTrue(TEXT("External 2x multiplies section 0.5x"), FMath::IsNearlyEqual(MI->GetPosition(), 0.5f, 0.001f));
    F.Settings->bEnabled = false;
    MI->SetPosition(0);
    F.Instance->Montage_Advance(0.25f);
    TestTrue(TEXT("Disabled settings preserve ordinary playback"), FMath::IsNearlyEqual(MI->GetPosition(), 0.5f, 0.001f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingRoutingTest, "DeckKnight.MontageRetiming.Playback.LoopsAndPartialEntry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingRoutingTest::RunTest(const FString&)
{
    FRetimingFixture F;
    auto* MI = F.Play();
    if (!TestNotNull(TEXT("Playing instance"), MI)) return false;
    MI->SetNextSectionName(TEXT("A"), TEXT("A"));
    F.Instance->Montage_Advance(4.5f);
    TestTrue(TEXT("Two full loops and half a second at 0.5x"), FMath::IsNearlyEqual(MI->GetPosition(), 0.25f, 0.001f));
    MI->JumpToSectionName(TEXT("B"));
    MI->SetPosition(1.5f);
    F.Instance->Montage_Advance(0.1f);
    TestTrue(TEXT("Partial entry retains B's 2x speed"), FMath::IsNearlyEqual(MI->GetPosition(), 1.7f, 0.001f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingIdentityTest, "DeckKnight.MontageRetiming.Data.StructuralEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingIdentityTest::RunTest(const FString&)
{
    FRetimingFixture F;
    auto* A = MontageRetiming::FindSection(*F.Montage, 0);
    auto* B = MontageRetiming::FindSection(*F.Montage, 1);
    const FGuid Identity = A->Identity;
    F.Montage->CompositeSections[0].SectionName = TEXT("Renamed");
    TestEqual(TEXT("Rename retains identity"), MontageRetiming::FindSection(*F.Montage, 0)->Identity, Identity);
    F.Montage->CompositeSections[1].SetTime(0.5f);
    auto Rows = MontageRetiming::Describe(*F.Montage);
    TestEqual(TEXT("Moving marker retains target seconds"), Rows[0].TargetSeconds, 2.0);
    TestTrue(TEXT("Moving marker recalculates speed"), FMath::IsNearlyEqual(Rows[0].Speed, 0.25));
    F.Montage->CompositeSections.Swap(0, 1);
    TestEqual(TEXT("Settings follow reordered section objects"), MontageRetiming::FindSection(*F.Montage, 0), B);
    F.Montage->CompositeSections.RemoveAt(0);
    Rows = MontageRetiming::Describe(*F.Montage);
    TestEqual(TEXT("Deletion preserves survivor target"), Rows[0].TargetSeconds, 2.0);
    TestTrue(TEXT("Deletion expands prior section and recomputes speed"), FMath::IsNearlyEqual(Rows[0].Speed, 1.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingValidationTest, "DeckKnight.MontageRetiming.Data.MultipleSlotsAndMixedSourceRates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingValidationTest::RunTest(const FString&)
{
    FRetimingFixture F;
    F.Skeleton->RegisterSlotNode(TEXT("UpperBody"));
    const FAnimSegment Copy = F.Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
    F.Montage->AddSlot(TEXT("UpperBody")).AnimTrack.AnimSegments.Add(Copy);
    auto Rows = MontageRetiming::Describe(*F.Montage);
    TestTrue(TEXT("Multiple DefaultGroup slots with matching rates are valid"), Rows[0].Error.IsEmpty());
    auto* Other = NewObject<UAnimSequence>();
    Other->SetSkeleton(F.Skeleton.Get());
    Other->GetController().InitializeModel();
    Other->GetController().SetFrameRate(FFrameRate(60, 1), false);
    Other->GetController().SetNumberOfFrames(FFrameNumber(120), false);
    Other->WaitOnExistingCompression();
    F.Montage->SlotAnimTracks[1].AnimTrack.AnimSegments[0].SetAnimReference(Other);
    Rows = MontageRetiming::Describe(*F.Montage);
    TestTrue(TEXT("Mixed source sample rates are rejected"), Rows[0].Error.Contains(TEXT("Mixed")));
    auto* MI = F.Play();
    if (TestNotNull(TEXT("Playing invalid montage still allowed"), MI))
    {
        F.Instance->Montage_Advance(0.25f);
        TestTrue(TEXT("Invalid retiming falls back to authored playback"), FMath::IsNearlyEqual(MI->GetPosition(), 0.25f, 0.001f));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingNotifyTest, "DeckKnight.MontageRetiming.Playback.NotifyAlignment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingNotifyTest::RunTest(const FString&)
{
    FRetimingFixture F;
    FAnimNotifyEvent& Notify = F.Montage->Notifies.AddDefaulted_GetRef();
    Notify.NotifyName = TEXT("Impact");
    Notify.TriggerWeightThreshold = 0.f;
    Notify.Link(F.Montage.Get(), 0.5f);
    auto* MI = F.Play();
    if (!TestNotNull(TEXT("Playing instance"), MI)) return false;
    MI->UpdateWeight(1.f);
    F.Instance->Montage_Advance(0.9f);
    TestEqual(TEXT("Notify not reached before one retimed second"), F.Instance->NotifyQueue.AnimNotifies.Num(), 0);
    F.Instance->Montage_Advance(0.2f);
    TestEqual(TEXT("Notify fires when its authored animation frame is traversed"), F.Instance->NotifyQueue.AnimNotifies.Num(), 1);
    TestEqual(TEXT("Notify's visible timeline placement is unchanged"), Notify.GetTime(), 0.5f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetimingCookedDataTest, "DeckKnight.MontageRetiming.Data.CookedSourceRates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetimingCookedDataTest::RunTest(const FString&)
{
    FRetimingFixture F;
    F.Settings->RefreshCookedData();
    TestTrue(TEXT("Valid source data is ready for cooked playback"), F.Settings->bValidForCookedPlayback);
    const auto EditorRows = MontageRetiming::Describe(*F.Montage);
    const auto CookedRows = MontageRetiming::Describe(*F.Montage, true);
    TestEqual(TEXT("Cooked frame rate retains source 30fps"), CookedRows[0].FramesPerSecond, 30.0);
    TestEqual(TEXT("Cooked slow section multiplier matches editor"), CookedRows[0].Speed, EditorRows[0].Speed);
    TestEqual(TEXT("Cooked fast section multiplier matches editor"), CookedRows[1].Speed, EditorRows[1].Speed);
    TestTrue(TEXT("Cooked data remains valid without consulting editor source data"), CookedRows[0].Error.IsEmpty());
    F.Montage->CompositeSections[1].SetTime(0);
    F.Settings->RefreshCookedData();
    TestFalse(TEXT("Invalid boundaries are rejected before cooking"), F.Settings->bValidForCookedPlayback);
    return true;
}
#endif
