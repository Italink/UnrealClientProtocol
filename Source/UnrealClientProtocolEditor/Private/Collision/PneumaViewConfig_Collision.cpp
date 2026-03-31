// MIT License - Copyright (c) 2025 Italink

#include "PneumaViewConfig_Collision.h"
#include "SCollisionStatsOverlay.h"
#include "Collision/PneumaViewCollision.h"

TSharedPtr<SWidget> UPneumaViewConfig_Collision::CreateViewportOverlay(
	TSharedRef<FAdvancedPreviewScene> PreviewScene,
	TWeakObjectPtr<UPneumaView> OwnerView)
{
	UPneumaViewCollision* CollisionView = Cast<UPneumaViewCollision>(OwnerView.Get());

	StatsOverlay = SNew(SCollisionStatsOverlay)
		.CollisionView(CollisionView);

	return StatsOverlay;
}
