// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/SceneCaptureComponent2D.h"
#include "PneumaViewConfig.generated.h"

class UPneumaView;
class FAdvancedPreviewScene;

UENUM(BlueprintType)
enum class EPneumaViewInteractionMode : uint8
{
	Orbit       UMETA(DisplayName = "Orbit"),
	FreeCamera  UMETA(DisplayName = "Free Camera"),
};

USTRUCT()
struct FPneumaViewTabInfo
{
	GENERATED_BODY()

	FName TabId;
	FText DisplayName;
	FName IconStyleName;
	float SizeCoefficient = 0.3f;
};

UCLASS(BlueprintType, Abstract)
class UPneumaViewConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	FVector2D InitialSize = FVector2D(1280.0, 720.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	EPneumaViewInteractionMode InteractionMode = EPneumaViewInteractionMode::Orbit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	FString DisplayName = TEXT("PneumaView");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	bool bShowFloor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	bool bShowGrid = true;

	virtual TArray<FPneumaViewTabInfo> GetAdditionalTabs() const { return {}; }

	virtual TSharedRef<SWidget> CreateTabWidget(
		const FName& TabId,
		TSharedRef<FAdvancedPreviewScene> PreviewScene,
		TWeakObjectPtr<UPneumaView> OwnerView);

	virtual TSharedPtr<SWidget> CreateViewportOverlay(
		TSharedRef<FAdvancedPreviewScene> PreviewScene,
		TWeakObjectPtr<UPneumaView> OwnerView) { return nullptr; }
};

UCLASS(BlueprintType)
class UPneumaViewConfig_Default : public UPneumaViewConfig
{
	GENERATED_BODY()

public:
	UPneumaViewConfig_Default()
	{
		InteractionMode = EPneumaViewInteractionMode::Orbit;
		InitialSize = FVector2D(1280.0, 720.0);
		DisplayName = TEXT("PneumaView");
	}
};

UCLASS(BlueprintType)
class UPneumaViewConfig_FreeCamera : public UPneumaViewConfig
{
	GENERATED_BODY()

public:
	UPneumaViewConfig_FreeCamera()
	{
		InteractionMode = EPneumaViewInteractionMode::FreeCamera;
		InitialSize = FVector2D(1280.0, 720.0);
		DisplayName = TEXT("PneumaView (Free)");
	}
};

// --- Capture Config ---

UCLASS(BlueprintType, Abstract)
class UPneumaCaptureConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaCapture")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaCapture")
	TEnumAsByte<ESceneCaptureSource> CaptureSource = SCS_FinalColorLDR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaCapture")
	FString ImageFormat = TEXT("png");
};

UCLASS(BlueprintType)
class UPneumaCaptureConfig_Default : public UPneumaCaptureConfig
{
	GENERATED_BODY()

public:
	UPneumaCaptureConfig_Default()
	{
		Resolution = FIntPoint(512, 512);
		CaptureSource = SCS_FinalColorLDR;
		ImageFormat = TEXT("png");
	}
};
