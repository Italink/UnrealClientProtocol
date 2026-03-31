// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class UPneumaViewCollision;

class SCollisionStatsOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCollisionStatsOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UPneumaViewCollision>, CollisionView)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshStats();

private:
	TWeakObjectPtr<UPneumaViewCollision> CollisionViewWeak;
	TSharedPtr<STextBlock> StatsText;
};
