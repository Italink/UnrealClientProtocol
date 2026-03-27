// MIT License - Copyright (c) 2025 Italink

#include "PneumaViewViewport.h"
#include "PneumaViewViewportClient.h"
#include "PneumaViewEditor.h"

#include "AdvancedPreviewScene.h"
#include "AdvancedPreviewSceneMenus.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"

#define LOCTEXT_NAMESPACE "SPneumaViewViewport"

SPneumaViewViewport::SPneumaViewViewport()
	: PreviewScene(MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())))
	, InteractionMode(EPneumaViewInteractionMode::Orbit)
{
}

SPneumaViewViewport::~SPneumaViewViewport()
{
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = nullptr;
	}
}

void SPneumaViewViewport::Construct(const FArguments& InArgs)
{
	PneumaViewEditorPtr = InArgs._PneumaViewEditor;
	InteractionMode = InArgs._InteractionMode;

	UWorld* World = PreviewScene->GetWorld();
	if (World != nullptr)
	{
		World->ChangeFeatureLevel(GWorld->GetFeatureLevel());
	}

	SEditorViewport::Construct(SEditorViewport::FArguments());

	UE::AdvancedPreviewScene::BindDefaultOnSettingsChangedHandler(PreviewScene, EditorViewportClient);
}

TSharedRef<SEditorViewport> SPneumaViewViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SPneumaViewViewport::GetExtenders() const
{
	return MakeShareable(new FExtender);
}

void SPneumaViewViewport::OnFloatingButtonClicked()
{
}

TSharedRef<FEditorViewportClient> SPneumaViewViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(
		new FPneumaViewViewportClient(SharedThis(this), PreviewScene.ToSharedRef(), InteractionMode));

	EditorViewportClient->bSetListenerPosition = false;
	EditorViewportClient->SetRealtime(true);
	EditorViewportClient->VisibilityDelegate.BindSP(this, &SPneumaViewViewport::IsVisible);

	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SPneumaViewViewport::BuildViewportToolbar()
{
	const FName ViewportToolbarName = "PneumaView.ViewportToolbar";

	if (!UToolMenus::Get()->IsMenuRegistered(ViewportToolbarName))
	{
		UToolMenu* ViewportToolbarMenu = UToolMenus::Get()->RegisterMenu(
			ViewportToolbarName, NAME_None, EMultiBoxType::SlimHorizontalToolBar);

		ViewportToolbarMenu->StyleName = "ViewportToolbar";

		{
			FToolMenuSection& LeftSection = ViewportToolbarMenu->AddSection("Left");
			LeftSection.AddEntry(UE::UnrealEd::CreateTransformsSubmenu());
			LeftSection.AddEntry(UE::UnrealEd::CreateSnappingSubmenu());
		}

		{
			FToolMenuSection& RightSection = ViewportToolbarMenu->AddSection("Right");
			RightSection.Alignment = EToolMenuSectionAlign::Last;

			RightSection.AddEntry(UE::UnrealEd::CreateCameraSubmenu(
				UE::UnrealEd::FViewportCameraMenuOptions().ShowAll()));

			{
				const FName ParentSubmenuName = "UnrealEd.ViewportToolbar.View";
				if (!UToolMenus::Get()->IsMenuRegistered(ParentSubmenuName))
				{
					UToolMenus::Get()->RegisterMenu(ParentSubmenuName);
				}
				UToolMenus::Get()->RegisterMenu("PneumaView.ViewportToolbar.ViewModes", ParentSubmenuName);
				RightSection.AddEntry(UE::UnrealEd::CreateViewModesSubmenu());
			}

			RightSection.AddEntry(UE::UnrealEd::CreateDefaultShowSubmenu());
			RightSection.AddEntry(UE::UnrealEd::CreatePerformanceAndScalabilitySubmenu());

			{
				const FName PreviewSceneMenuName = "PneumaView.ViewportToolbar.AssetViewerProfile";
				RightSection.AddEntry(UE::UnrealEd::CreateAssetViewerProfileSubmenu());
				UE::AdvancedPreviewScene::Menus::ExtendAdvancedPreviewSceneSettings(PreviewSceneMenuName);
				UE::UnrealEd::ExtendPreviewSceneSettingsWithTabEntry(PreviewSceneMenuName);
			}
		}
	}

	FToolMenuContext ViewportToolbarContext;
	{
		ViewportToolbarContext.AppendCommandList(PreviewScene->GetCommandList());
		ViewportToolbarContext.AppendCommandList(GetCommandList());

		UUnrealEdViewportToolbarContext* ContextObject =
			UE::UnrealEd::CreateViewportToolbarDefaultContext(SharedThis(this));
		ContextObject->bShowCoordinateSystemControls = false;
		ContextObject->AssetEditorToolkit = PneumaViewEditorPtr;
		ContextObject->PreviewSettingsTabId = FPneumaViewEditor::PreviewSceneSettingsTabId;

		ViewportToolbarContext.AddObject(ContextObject);
	}

	return UToolMenus::Get()->GenerateWidget(ViewportToolbarName, ViewportToolbarContext);
}

FEditorViewportClient* SPneumaViewViewport::GetViewportClient() const
{
	return EditorViewportClient.Get();
}

EVisibility SPneumaViewViewport::OnGetViewportContentVisibility() const
{
	return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SPneumaViewViewport::IsVisible() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
