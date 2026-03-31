// MIT License - Copyright (c) 2025 Italink

#include "PneumaView/PneumaViewConfig.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UPneumaViewConfig::CreateTabWidget(
	const FName& TabId,
	TSharedRef<FAdvancedPreviewScene> PreviewScene,
	TWeakObjectPtr<UPneumaView> OwnerView)
{
	return SNullWidget::NullWidget;
}
