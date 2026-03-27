// MIT License - Copyright (c) 2025 Italink

#include "PneumaView/PneumaView.h"
#include "PneumaView/PneumaViewConfig.h"
#include "PneumaViewEditor.h"
#include "PneumaViewViewport.h"
#include "EditorViewportClient.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "ImageUtils.h"
#include "AdvancedPreviewScene.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

#define LOCTEXT_NAMESPACE "UPneumaView"

namespace PneumaViewPrivate
{
	static const TMap<FString, EViewModeIndex> ViewModeMap = {
		{ TEXT("Lit"),                  VMI_Lit },
		{ TEXT("Unlit"),                VMI_Unlit },
		{ TEXT("Wireframe"),            VMI_Wireframe },
		{ TEXT("BrushWireframe"),       VMI_BrushWireframe },
		{ TEXT("DetailLighting"),       VMI_Lit_DetailLighting },
		{ TEXT("LightingOnly"),         VMI_LightingOnly },
		{ TEXT("LightComplexity"),      VMI_LightComplexity },
		{ TEXT("ShaderComplexity"),     VMI_ShaderComplexity },
		{ TEXT("LightmapDensity"),      VMI_LightmapDensity },
		{ TEXT("Reflections"),          VMI_ReflectionOverride },
		{ TEXT("CollisionPawn"),        VMI_CollisionPawn },
		{ TEXT("CollisionVisibility"),  VMI_CollisionVisibility },
		{ TEXT("PathTracing"),          VMI_PathTracing },
		{ TEXT("RayTracingDebug"),      VMI_RayTracingDebug },
		{ TEXT("LitWireframe"),         VMI_Lit_Wireframe },
	};

	static FString ViewModeToString(EViewModeIndex Mode)
	{
		for (const auto& Pair : ViewModeMap)
		{
			if (Pair.Value == Mode) return Pair.Key;
		}
		return TEXT("Unknown");
	}
}

TMap<FString, TObjectPtr<UPneumaView>> UPneumaView::Instances;

UPneumaView* UPneumaView::FindOrCreate(const FString& InUniqueName, const FString& ConfigType)
{
	if (TObjectPtr<UPneumaView>* Existing = Instances.Find(InUniqueName))
	{
		UPneumaView* ExistingView = Existing->Get();
		if (ExistingView && ExistingView->Editor.IsValid())
		{
			ExistingView->Editor->FocusWindow();
			return ExistingView;
		}
		if (ExistingView)
		{
			ExistingView->RemoveFromRoot();
		}
		Instances.Remove(InUniqueName);
	}

	FString FullClassName = FString::Printf(TEXT("/Script/UnrealClientProtocolEditor.%s"), *ConfigType);
	UClass* ConfigClass = FindObject<UClass>(nullptr, *FullClassName);

	if (!ConfigClass)
	{
		ConfigClass = LoadObject<UClass>(nullptr, *FullClassName);
	}

	if (!ConfigClass || !ConfigClass->IsChildOf(UPneumaViewConfig::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("PneumaView: ConfigType '%s' not found or not a UPneumaViewConfig subclass."), *ConfigType);
		return nullptr;
	}

	FName ConfigObjectName = *InUniqueName;
	UPneumaViewConfig* ConfigInstance = NewObject<UPneumaViewConfig>(
		GetTransientPackage(), ConfigClass, ConfigObjectName, RF_Transient);
	if (!ConfigInstance)
	{
		return nullptr;
	}

	ConfigInstance->DisplayName = InUniqueName;

	UPneumaView* NewView = NewObject<UPneumaView>(GetTransientPackage(), NAME_None, RF_Transient);
	NewView->AddToRoot();
	NewView->UniqueName = InUniqueName;
	NewView->Config = ConfigInstance;

	TSharedRef<FPneumaViewEditor> NewEditor = MakeShared<FPneumaViewEditor>();
	NewView->Editor = NewEditor;

	NewEditor->OnEditorClosed.BindUObject(NewView, &UPneumaView::OnEditorClosed);
	NewEditor->InitPneumaViewEditor(nullptr, ConfigInstance);

	Instances.Add(InUniqueName, NewView);

	return NewView;
}

bool UPneumaView::Close()
{
	if (!Instances.Contains(UniqueName))
	{
		return false;
	}

	if (Editor.IsValid())
	{
		Editor->OnEditorClosed.Unbind();
		Editor->CloseWindow(EAssetEditorCloseReason::CloseAllEditorsForAsset);
		Editor.Reset();
	}

	Instances.Remove(UniqueName);
	RemoveFromRoot();

	return true;
}

void UPneumaView::OnEditorClosed()
{
	Editor.Reset();
	Instances.Remove(UniqueName);
	RemoveFromRoot();
}

bool UPneumaView::SetViewMode(const FString& ModeName)
{
	if (!Editor.IsValid()) return false;

	TSharedPtr<SPneumaViewViewport> Viewport = Editor->GetViewport();
	if (!Viewport.IsValid()) return false;

	FEditorViewportClient* Client = Viewport->GetViewportClient();
	if (!Client) return false;

	const EViewModeIndex* Found = PneumaViewPrivate::ViewModeMap.Find(ModeName);
	if (!Found)
	{
		UE_LOG(LogTemp, Warning, TEXT("PneumaView: Unknown ViewMode '%s'. Valid: Lit, Unlit, Wireframe, BrushWireframe, DetailLighting, LightingOnly, LightComplexity, ShaderComplexity, LightmapDensity, Reflections, CollisionPawn, CollisionVisibility, PathTracing, RayTracingDebug, LitWireframe"), *ModeName);
		return false;
	}

	Client->SetViewMode(*Found);
	return true;
}

FString UPneumaView::GetViewMode()
{
	if (!Editor.IsValid()) return TEXT("Unknown");

	TSharedPtr<SPneumaViewViewport> Viewport = Editor->GetViewport();
	if (!Viewport.IsValid()) return TEXT("Unknown");

	FEditorViewportClient* Client = Viewport->GetViewportClient();
	if (!Client) return TEXT("Unknown");

	return PneumaViewPrivate::ViewModeToString(Client->GetViewMode());
}

TArray<FString> UPneumaView::CaptureViewport(const TArray<FPneumaCameraPose>& CameraPoses, const FString& CaptureConfigType)
{
	TArray<FString> Results;

	if (!Editor.IsValid() || CameraPoses.IsEmpty())
	{
		return Results;
	}

	TSharedPtr<SPneumaViewViewport> Viewport = Editor->GetViewport();
	if (!Viewport.IsValid())
	{
		return Results;
	}

	UWorld* PreviewWorld = Viewport->GetPreviewScene()->GetWorld();
	if (!PreviewWorld)
	{
		return Results;
	}

	FString FullClassName = FString::Printf(TEXT("/Script/UnrealClientProtocolEditor.%s"), *CaptureConfigType);
	UClass* CaptureConfigClass = FindObject<UClass>(nullptr, *FullClassName);
	if (!CaptureConfigClass)
	{
		CaptureConfigClass = LoadObject<UClass>(nullptr, *FullClassName);
	}
	if (!CaptureConfigClass || !CaptureConfigClass->IsChildOf(UPneumaCaptureConfig::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("PneumaView: CaptureConfigType '%s' not found or not a UPneumaCaptureConfig subclass."), *CaptureConfigType);
		return Results;
	}

	const UPneumaCaptureConfig* CaptureConfig = CaptureConfigClass->GetDefaultObject<UPneumaCaptureConfig>();

	const int32 TileW = CaptureConfig->Resolution.X;
	const int32 TileH = CaptureConfig->Resolution.Y;
	const int32 NumPoses = CameraPoses.Num();
	const int32 Cols = FMath::CeilToInt32(FMath::Sqrt(static_cast<float>(NumPoses)));
	const int32 Rows = FMath::CeilToInt32(static_cast<float>(NumPoses) / Cols);

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	RenderTarget->InitAutoFormat(TileW, TileH);
	RenderTarget->UpdateResourceImmediate(true);

	ASceneCapture2D* CaptureActor = PreviewWorld->SpawnActor<ASceneCapture2D>();
	CaptureActor->SetFlags(RF_Transient);
	USceneCaptureComponent2D* CaptureComp = CaptureActor->GetCaptureComponent2D();
	CaptureComp->TextureTarget = RenderTarget;
	CaptureComp->CaptureSource = CaptureConfig->CaptureSource;
	CaptureComp->bCaptureEveryFrame = false;
	CaptureComp->bCaptureOnMovement = false;
	CaptureComp->bAlwaysPersistRenderingState = true;

	TArray<FColor> CompositePixels;
	const int32 CompositeW = TileW * Cols;
	const int32 CompositeH = TileH * Rows;
	CompositePixels.SetNumZeroed(CompositeW * CompositeH);

	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();

	for (int32 i = 0; i < NumPoses; i++)
	{
		const FPneumaCameraPose& Pose = CameraPoses[i];
		CaptureComp->SetWorldLocationAndRotation(Pose.Location, Pose.Rotation);
		CaptureComp->CaptureScene();

		TArray<FColor> TilePixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm, CubeFace_MAX);
		FIntRect ReadRegion(0, 0, TileW, TileH);
		RTResource->ReadPixels(TilePixels, ReadFlags, ReadRegion);

		const int32 Col = i % Cols;
		const int32 Row = i / Cols;
		const int32 OffsetX = Col * TileW;
		const int32 OffsetY = Row * TileH;

		for (int32 Y = 0; Y < TileH; Y++)
		{
			FMemory::Memcpy(
				&CompositePixels[(OffsetY + Y) * CompositeW + OffsetX],
				&TilePixels[Y * TileW],
				TileW * sizeof(FColor));
		}
	}

	CaptureActor->Destroy();
	RenderTarget->MarkAsGarbage();

	FString OutputDir = FPaths::ProjectSavedDir() / TEXT("PneumaView") / UniqueName;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutputDir))
	{
		PlatformFile.CreateDirectoryTree(*OutputDir);
	}

	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	FString OutputPath = OutputDir / FString::Printf(TEXT("%s.%s"), *Timestamp, *CaptureConfig->ImageFormat);

	FImageView ImageView(CompositePixels.GetData(), CompositeW, CompositeH);
	if (FImageUtils::SaveImageByExtension(*OutputPath, ImageView))
	{
		Results.Add(OutputPath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PneumaView: Failed to save capture to '%s'"), *OutputPath);
	}

	return Results;
}

FString UPneumaView::GetPreviewWorld()
{
	if (!Editor.IsValid())
	{
		return FString();
	}

	TSharedPtr<SPneumaViewViewport> Viewport = Editor->GetViewport();
	if (!Viewport.IsValid())
	{
		return FString();
	}

	UWorld* World = Viewport->GetPreviewScene()->GetWorld();
	if (!World)
	{
		return FString();
	}

	return World->GetPathName();
}

#undef LOCTEXT_NAMESPACE
