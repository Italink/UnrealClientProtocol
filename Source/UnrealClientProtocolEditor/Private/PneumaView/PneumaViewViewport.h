// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "AdvancedPreviewScene.h"
#include "PneumaView/PneumaViewConfig.h"

class FPneumaViewEditor;
class FPneumaViewViewportClient;

class SPneumaViewViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SPneumaViewViewport) {}
		SLATE_ARGUMENT(EPneumaViewInteractionMode, InteractionMode)
		SLATE_ARGUMENT(bool, bShowGrid)
		SLATE_ARGUMENT(bool, bShowFloor)
		SLATE_ARGUMENT(TWeakPtr<FPneumaViewEditor>, PneumaViewEditor)
	SLATE_END_ARGS()

	SPneumaViewViewport();
	virtual ~SPneumaViewViewport() override;

	void Construct(const FArguments& InArgs);

	TSharedRef<FAdvancedPreviewScene> GetPreviewScene() { return PreviewScene.ToSharedRef(); }
	FEditorViewportClient* GetViewportClient() const;

	void SetOverlay(TSharedPtr<SWidget> InOverlay);

	// ICommonEditorViewportToolbarInfoProvider
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> BuildViewportToolbar() override;
	virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
	virtual EVisibility OnGetViewportContentVisibility() const override;
	virtual bool IsVisible() const override;

private:
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FPneumaViewViewportClient> EditorViewportClient;
	TWeakPtr<FPneumaViewEditor> PneumaViewEditorPtr;
	EPneumaViewInteractionMode InteractionMode;
	bool bShowGrid = true;
	bool bShowFloor = true;
	TSharedPtr<SWidget> OverlayWidget;
};
