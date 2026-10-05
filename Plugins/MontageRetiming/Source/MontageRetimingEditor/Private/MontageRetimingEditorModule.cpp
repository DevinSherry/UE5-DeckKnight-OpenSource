#include "Modules/ModuleManager.h"
#include "IAnimationEditorModule.h"
#include "IAnimationEditor.h"
#include "IPersonaToolkit.h"
#include "Animation/AnimMontage.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "SMontageRetiming.h"
#include "MontageRetimingPreviewInstance.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Containers/Ticker.h"
#include "UObject/UObjectIterator.h"

class FMontageRetimingEditorModule : public IModuleInterface
{
    FDelegateHandle ExtenderHandle;
    FTSTicker::FDelegateHandle PreviewTicker;
    FDelegateHandle ObjectModifiedHandle;
public:
    virtual bool SupportsDynamicReloading() override { return false; }
    virtual void StartupModule() override
    {
        ObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddLambda([](UObject* Object)
        {
            for (TObjectIterator<UMontageRetimingPreviewInstance> It; It; ++It)
                if (IsValid(*It)) It->HandleExternalEdit(Object);
        });
        PreviewTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
        {
            for (TObjectIterator<UDebugSkelMeshComponent> It; It; ++It)
                if (IsValid(*It) && It->PreviewInstance)
                    if (auto* M = Cast<UAnimMontage>(It->PreviewInstance->GetCurrentAsset()))
                        if (MontageRetiming::IsEnabled(*M)) UMontageRetimingPreviewInstance::Install(**It);
            return true;
        }), 0.1f);
        auto& Module = FModuleManager::LoadModuleChecked<IAnimationEditorModule>("AnimationEditor");
        auto Delegate = IAnimationEditorModule::FAnimationEditorToolbarExtender::CreateLambda(
            [](const TSharedRef<FUICommandList> Commands, TSharedRef<IAnimationEditor> Editor)
            {
                auto Extender = MakeShared<FExtender>();
                TWeakPtr<IAnimationEditor> WeakEditor = Editor;
                Extender->AddToolBarExtension("Asset", EExtensionHook::After, Commands,
                    FToolBarExtensionDelegate::CreateLambda([WeakEditor](FToolBarBuilder& Builder)
                    {
                        Builder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([WeakEditor]
                        {
                            auto Pinned = WeakEditor.Pin();
                            if (!Pinned) return;
                            const FName TabId(TEXT("MontageRetiming"));
                            auto Manager = Pinned->GetTabManager();
                            if (!Manager->HasTabSpawner(TabId))
                                Manager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateLambda([WeakEditor](const FSpawnTabArgs&)
                                {
                                    return SNew(SDockTab).Label(FText::FromString(TEXT("Section Retiming")))
                                        [ SNew(SMontageRetiming).Editor(WeakEditor) ];
                                })).SetDisplayName(FText::FromString(TEXT("Section Retiming")));
                            Manager->TryInvokeTab(TabId);
                        }), FCanExecuteAction::CreateLambda([WeakEditor]
                        {
                            auto Pinned = WeakEditor.Pin();
                            return Pinned && Cast<UAnimMontage>(Pinned->GetPersonaToolkit()->GetAnimationAsset()) != nullptr;
                        })), NAME_None, FText::FromString(TEXT("Section Retiming")),
                            FText::FromString(TEXT("Adjust section playback durations without moving the montage timeline.")), FSlateIcon());
                    }));
                return Extender;
            });
        ExtenderHandle = Delegate.GetHandle();
        Module.GetAllAnimationEditorToolbarExtenders().Add(Delegate);
    }
    virtual void ShutdownModule() override
    {
        FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);
        FTSTicker::GetCoreTicker().RemoveTicker(PreviewTicker);
        if (auto* Module = FModuleManager::GetModulePtr<IAnimationEditorModule>("AnimationEditor"))
            Module->GetAllAnimationEditorToolbarExtenders().RemoveAll([this](const auto& D) { return D.GetHandle() == ExtenderHandle; });
    }
};

IMPLEMENT_MODULE(FMontageRetimingEditorModule, MontageRetimingEditor)
