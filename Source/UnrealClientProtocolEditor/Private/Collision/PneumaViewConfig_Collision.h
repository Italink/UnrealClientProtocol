// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "PneumaView/PneumaViewConfig.h"
#include "PneumaViewConfig_Collision.generated.h"

class UPneumaViewCollision;

UCLASS(BlueprintType)
class UPneumaViewConfig_Collision : public UPneumaViewConfig
{
	GENERATED_BODY()

public:
	UPneumaViewConfig_Collision()
	{
		InteractionMode = EPneumaViewInteractionMode::Orbit;
		InitialSize = FVector2D(1600.0, 900.0);
		DisplayName = TEXT("Collision Generator");
		bShowFloor = true;
		bShowGrid = true;
	}

	virtual TSharedPtr<SWidget> CreateViewportOverlay(
		TSharedRef<FAdvancedPreviewScene> PreviewScene,
		TWeakObjectPtr<UPneumaView> OwnerView) override;

	TSharedPtr<class SCollisionStatsOverlay> StatsOverlay;
};
