// MIT License - Copyright (c) 2025 Italink

#include "PneumaViewEditor.h"
#include "PneumaViewViewport.h"
#include "PneumaView/PneumaView.h"

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
	UPneumaViewConfig* InConfig,
	TWeakObjectPtr<UPneumaView> InOwnerView)
{
	Config = InConfig;
	OwnerView = InOwnerView;

	if (Config)
	{
		AdditionalTabs = Config->GetAdditionalTabs();
	}

	auto RightPanel = FTabManager::NewStack()
		->SetSizeCoefficient(0.3f)
		->AddTab(PreviewSceneSettingsTabId, ETabState::OpenedTab);

	for (const FPneumaViewTabInfo& TabInfo : AdditionalTabs)
	{
		RightPanel->AddTab(TabInfo.TabId, ETabState::OpenedTab);
	}

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout =
		FTabManager::NewLayout("Standalone_PneumaViewEditor_Layout_v2")
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
				->Split(RightPanel)
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

	for (const FPneumaViewTabInfo& TabInfo : AdditionalTabs)
	{
		InTabManager->RegisterTabSpawner(TabInfo.TabId,
				FOnSpawnTab::CreateSP(this, &FPneumaViewEditor::SpawnTab_Additional, TabInfo.TabId))
			.SetDisplayName(TabInfo.DisplayName)
			.SetGroup(WorkspaceMenuCategoryRef)
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(),
				TabInfo.IconStyleName.IsNone() ? FName("LevelEditor.Tabs.Details") : TabInfo.IconStyleName));
	}
}

void FPneumaViewEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(PreviewSceneSettingsTabId);

	for (const FPneumaViewTabInfo& TabInfo : AdditionalTabs)
	{
		InTabManager->UnregisterTabSpawner(TabInfo.TabId);
	}
}

TSharedRef<SDockTab> FPneumaViewEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel", "Viewport"));

	Viewport = SNew(SPneumaViewViewport)
		.InteractionMode(Config ? Config->InteractionMode : EPneumaViewInteractionMode::Orbit)
		.bShowGrid(Config ? Config->bShowGrid : true)
		.bShowFloor(Config ? Config->bShowFloor : true)
		.PneumaViewEditor(StaticCastSharedRef<FPneumaViewEditor>(AsShared()));

	if (Config)
	{
		TSharedPtr<SWidget> Overlay = Config->CreateViewportOverlay(
			Viewport->GetPreviewScene(), OwnerView);
		if (Overlay.IsValid())
		{
			Viewport->SetOverlay(Overlay);
		}
	}

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

TSharedRef<SDockTab> FPneumaViewEditor::SpawnTab_Additional(const FSpawnTabArgs& Args, FName TabId)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab);

	if (Config && Viewport.IsValid())
	{
		DockTab->SetContent(Config->CreateTabWidget(TabId, Viewport->GetPreviewScene(), OwnerView));
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
