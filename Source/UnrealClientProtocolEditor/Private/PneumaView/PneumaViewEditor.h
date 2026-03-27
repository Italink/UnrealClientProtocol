// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "PneumaView/PneumaViewConfig.h"

class SPneumaViewViewport;

DECLARE_DELEGATE(FOnPneumaViewEditorClosed);

class FPneumaViewEditor : public FAssetEditorToolkit
{
public:
	static const FName ViewportTabId;
	static const FName PreviewSceneSettingsTabId;

	void InitPneumaViewEditor(const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UPneumaViewConfig* InConfig);

	virtual ~FPneumaViewEditor() override;

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void OnClose() override;

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	TSharedPtr<SPneumaViewViewport> GetViewport() const { return Viewport; }

	FOnPneumaViewEditorClosed OnEditorClosed;

private:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewSceneSettings(const FSpawnTabArgs& Args);

	TSharedPtr<SPneumaViewViewport> Viewport;
	TSharedPtr<SWidget> AdvancedPreviewSettingsWidget;
	TObjectPtr<UPneumaViewConfig> Config;
};
