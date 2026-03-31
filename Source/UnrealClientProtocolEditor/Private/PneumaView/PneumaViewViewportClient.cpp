// MIT License - Copyright (c) 2025 Italink

#include "PneumaViewViewportClient.h"

#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "PneumaViewViewport.h"

#define LOCTEXT_NAMESPACE "FPneumaViewViewportClient"

FPneumaViewViewportClient::FPneumaViewViewportClient(
	const TSharedRef<SPneumaViewViewport>& InViewport,
	const TSharedRef<FAdvancedPreviewScene>& InPreviewScene,
	EPneumaViewInteractionMode InInteractionMode,
	bool bInShowGrid)
	: FEditorViewportClient(nullptr, &InPreviewScene.Get(), StaticCastSharedRef<SEditorViewport>(InViewport))
	, InteractionMode(InInteractionMode)
{
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = bInShowGrid;
	DrawHelper.GridColorAxis = FColor(160, 160, 160);
	DrawHelper.GridColorMajor = FColor(144, 144, 144);
	DrawHelper.GridColorMinor = FColor(128, 128, 128);
	DrawHelper.PerspectiveGridSize = 2048.0f;
	DrawHelper.NumCells = FMath::FloorToInt32(DrawHelper.PerspectiveGridSize / (16 * 2));

	SetViewMode(VMI_Lit);

	EngineShowFlags.SetSeparateTranslucency(true);
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.SetCompositeEditorPrimitives(true);
	OverrideNearClipPlane(1.0f);

	bUsingOrbitCamera = (InteractionMode == EPneumaViewInteractionMode::Orbit);
}

bool FPneumaViewViewportClient::ShouldOrbitCamera() const
{
	if (InteractionMode == EPneumaViewInteractionMode::Orbit)
	{
		return true;
	}
	return FEditorViewportClient::ShouldOrbitCamera();
}

#undef LOCTEXT_NAMESPACE
