// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "PneumaView/PneumaViewConfig.h"

class FAdvancedPreviewScene;
class SPneumaViewViewport;

class FPneumaViewViewportClient : public FEditorViewportClient, public TSharedFromThis<FPneumaViewViewportClient>
{
public:
	FPneumaViewViewportClient(const TSharedRef<SPneumaViewViewport>& InViewport,
		const TSharedRef<FAdvancedPreviewScene>& InPreviewScene,
		EPneumaViewInteractionMode InInteractionMode);

	virtual bool ShouldOrbitCamera() const override;

private:
	EPneumaViewInteractionMode InteractionMode;
};
