// MIT License - Copyright (c) 2025 Italink

#include "PneumaViewEditor.h"
#include "PneumaViewViewport.h"

#include "AdvancedPreviewSceneModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FPneumaViewEditor"

static const FName PneumaViewEditorAppIdentifier(TEXT("PneumaViewEditor"));

const FName FPneumaViewEditor::ViewportTabId(TEXT("PneumaViewEditor_Viewport"));
const FName FPneumaViewEditor::PreviewSceneSettingsTabId(TEXT("PneumaViewEditor_PreviewScene"));

void FPneumaViewEditor::InitPneumaViewEditor(
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UPneumaViewConfig* InConfig)
{
	Config = InConfig;

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout =
		FTabManager::NewLayout("Standalone_PneumaViewEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.7f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(PreviewSceneSettingsTabId, ETabState::OpenedTab)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		EToolkitMode::Standalone,
		InitToolkitHost,
		PneumaViewEditorAppIdentifier,
		StandaloneDefaultLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		InConfig);

	RegenerateMenusAndToolbars();
}

FPneumaViewEditor::~FPneumaViewEditor()
{
}

void FPneumaViewEditor::OnClose()
{
	OnEditorClosed.ExecuteIfBound();
}

void FPneumaViewEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu_PneumaViewEditor", "PneumaView Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId,
			FOnSpawnTab::CreateSP(this, &FPneumaViewEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(PreviewSceneSettingsTabId,
			FOnSpawnTab::CreateSP(this, &FPneumaViewEditor::SpawnTab_PreviewSceneSettings))
		.SetDisplayName(LOCTEXT("PreviewSceneTab", "Preview Scene Settings"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FPneumaViewEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(PreviewSceneSettingsTabId);
}

TSharedRef<SDockTab> FPneumaViewEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel", "Viewport"));

	Viewport = SNew(SPneumaViewViewport)
		.InteractionMode(Config ? Config->InteractionMode : EPneumaViewInteractionMode::Orbit)
		.PneumaViewEditor(StaticCastSharedRef<FPneumaViewEditor>(AsShared()));

	DockTab->SetContent(Viewport.ToSharedRef());
	return DockTab;
}

TSharedRef<SDockTab> FPneumaViewEditor::SpawnTab_PreviewSceneSettings(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.Label(LOCTEXT("PreviewSceneSettingsLabel", "Preview Scene Settings"));

	if (Viewport.IsValid())
	{
		FAdvancedPreviewSceneModule& AdvancedPreviewSceneModule =
			FModuleManager::LoadModuleChecked<FAdvancedPreviewSceneModule>("AdvancedPreviewScene");

		AdvancedPreviewSettingsWidget = AdvancedPreviewSceneModule.CreateAdvancedPreviewSceneSettingsWidget(
			Viewport->GetPreviewScene());
	}

	if (AdvancedPreviewSettingsWidget.IsValid())
	{
		DockTab->SetContent(AdvancedPreviewSettingsWidget.ToSharedRef());
	}
	else
	{
		DockTab->SetContent(SNullWidget::NullWidget);
	}

	return DockTab;
}

FName FPneumaViewEditor::GetToolkitFName() const
{
	return FName("PneumaViewEditor");
}

FText FPneumaViewEditor::GetBaseToolkitName() const
{
	if (Config && !Config->DisplayName.IsEmpty())
	{
		return FText::FromString(Config->DisplayName);
	}
	return LOCTEXT("ToolkitName", "PneumaView");
}

FString FPneumaViewEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "PneumaView ").ToString();
}

FLinearColor FPneumaViewEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.8f, 0.5f);
}

#undef LOCTEXT_NAMESPACE
