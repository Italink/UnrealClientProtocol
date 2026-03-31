// MIT License - Copyright (c) 2025 Italink

#include "Collision/PneumaViewCollision.h"
#include "PneumaViewConfig_Collision.h"
#include "CollisionMeshAlgorithm.h"
#include "SCollisionStatsOverlay.h"
#include "PneumaView/PneumaViewEditor.h"
#include "PneumaView/PneumaViewConfig.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/ScopedSlowTask.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "EditorAssetLibrary.h"

using namespace UE::Geometry;

DEFINE_LOG_CATEGORY_STATIC(LogCollisionGen, Log, All);

static FString CollisionJsonToString(const TSharedRef<FJsonObject>& Obj)
{
	FString Out;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Obj, Writer);
	return Out;
}

// ============================================================================
// Open
// ============================================================================

UPneumaViewCollision* UPneumaViewCollision::Open(UStaticMesh* InSourceMesh)
{
	if (!InSourceMesh)
	{
		UE_LOG(LogCollisionGen, Error, TEXT("Open: null SourceMesh"));
		return nullptr;
	}

	FString ViewName = FString::Printf(TEXT("Collision_%s"), *InSourceMesh->GetName());

	if (TObjectPtr<UPneumaView>* Existing = Instances.Find(ViewName))
	{
		UPneumaViewCollision* ExistingView = Cast<UPneumaViewCollision>(Existing->Get());
		if (ExistingView && ExistingView->GetEditor().IsValid())
		{
			ExistingView->GetEditor()->FocusWindow();
			return ExistingView;
		}
		if (Existing->Get())
		{
			Existing->Get()->RemoveFromRoot();
		}
		Instances.Remove(ViewName);
	}

	UPneumaViewConfig_Collision* ConfigInstance = NewObject<UPneumaViewConfig_Collision>(
		GetTransientPackage(), NAME_None, RF_Transient);
	ConfigInstance->DisplayName = ViewName;

	UPneumaViewCollision* NewView = NewObject<UPneumaViewCollision>(
		GetTransientPackage(), NAME_None, RF_Transient);
	NewView->SourceMesh = InSourceMesh;
	NewView->InitFromConfig(ViewName, ConfigInstance);
	NewView->SetupPreviewScene();

	NewView->CachedAnalysis = NewView->Analyze();

	return NewView;
}

// ============================================================================
// SetupPreviewScene
// ============================================================================

void UPneumaViewCollision::SetupPreviewScene()
{
	if (!SourceMesh) return;

	FBox Bounds = SourceMesh->GetBoundingBox();
	FVector BoundsSize = Bounds.GetSize();
	double Spacing = FMath::Max(BoundsSize.X, BoundsSize.Y) * 1.5;

	FVector LeftPos = FVector(-Spacing * 0.5, 0, 0);
	FVector RightPos = FVector(Spacing * 0.5, 0, 0);

	SourceActor = SpawnPreviewMeshActor(SourceMesh, LeftPos);
	CollisionActor = SpawnPreviewMeshActor(SourceMesh, RightPos);

	FocusOnAll();
}

// ============================================================================
// Analyze
// ============================================================================

FString UPneumaViewCollision::Analyze()
{
	if (!SourceMesh) return TEXT("{}");

	FMeshDescription MeshDesc;
	FStaticMeshAttributes(MeshDesc).Register();

	const FStaticMeshSourceModel& SrcModel = SourceMesh->GetSourceModel(0);
	if (SourceMesh->GetMeshDescription(0))
	{
		MeshDesc = *SourceMesh->GetMeshDescription(0);
	}
	else
	{
		return TEXT("{\"error\":\"No mesh description at LOD0\"}");
	}

	FDynamicMesh3 DynMesh;
	FMeshDescriptionToDynamicMesh Converter;
	Converter.Convert(&MeshDesc, DynMesh);

	CachedAnalysis = FCollisionMeshAlgorithm::AnalyzeMesh(DynMesh);
	return CachedAnalysis;
}

// ============================================================================
// Generate (async via DeferredResponse)
// ============================================================================

FUCPDeferredResponse UPneumaViewCollision::Generate(const FCollisionGenParams& Params)
{
	FUCPDeferredResponse Deferred;
	LastGenParams = Params;

	if (!SourceMesh)
	{
		auto Err = MakeShared<FJsonObject>();
		Err->SetStringField(TEXT("error"), TEXT("No source mesh"));
		Deferred.Complete(Err);
		return Deferred;
	}

	// Read mesh on GameThread
	FMeshDescription MeshDesc;
	FStaticMeshAttributes(MeshDesc).Register();
	if (SourceMesh->GetMeshDescription(0))
	{
		MeshDesc = *SourceMesh->GetMeshDescription(0);
	}
	else
	{
		auto Err = MakeShared<FJsonObject>();
		Err->SetStringField(TEXT("error"), TEXT("No mesh description at LOD0"));
		Deferred.Complete(Err);
		return Deferred;
	}

	TSharedPtr<FDynamicMesh3> SourceDynMesh = MakeShared<FDynamicMesh3>();
	{
		FMeshDescriptionToDynamicMesh Converter;
		Converter.Convert(&MeshDesc, *SourceDynMesh);
	}
	SourceDynMesh->DiscardAttributes();

	FCollisionGenParams ParamsCopy = Params;
	TWeakObjectPtr<UPneumaViewCollision> WeakThis(this);
	FUCPDeferredResponse Captured = Deferred;

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, SourceDynMesh, ParamsCopy, Captured]()
	{
		FCollisionMeshResult Result = FCollisionMeshAlgorithm::Generate(*SourceDynMesh, ParamsCopy);

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Result = MoveTemp(Result), Captured]() mutable
		{
			UPneumaViewCollision* Self = WeakThis.Get();
			if (!Self)
			{
				auto Err = MakeShared<FJsonObject>();
				Err->SetStringField(TEXT("error"), TEXT("View was closed during generation"));
				Captured.Complete(Err);
				return;
			}

			Self->LastGenElapsed = Result.ElapsedSeconds;

			// Create or update temp collision mesh asset
			FString TempPath = FString::Printf(TEXT("/Game/_Temp/PneumaCollision/%s_Collision"),
				*Self->SourceMesh->GetName());

			if (Self->TempCollisionMesh)
			{
				// Update existing
				FMeshDescription OutMeshDesc;
				FStaticMeshAttributes(OutMeshDesc).Register();
				FDynamicMeshToMeshDescription MeshConverter;
				MeshConverter.Convert(&Result.ResultMesh, OutMeshDesc);

				Self->TempCollisionMesh->GetSourceModel(0).BuildSettings.bRecomputeNormals = true;
				Self->TempCollisionMesh->GetSourceModel(0).BuildSettings.bRecomputeTangents = true;
				Self->TempCollisionMesh->CreateMeshDescription(0, MoveTemp(OutMeshDesc));
				Self->TempCollisionMesh->CommitMeshDescription(0);
				Self->TempCollisionMesh->Build(false);
				Self->TempCollisionMesh->PostEditChange();
			}
			else
			{
				// Create new transient mesh
				FString PackageName = TempPath;
				FString AssetName = FPaths::GetBaseFilename(TempPath);
				UPackage* Package = CreatePackage(*PackageName);
				Package->SetFlags(RF_Transient);

				UStaticMesh* NewMesh = NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transient);
				NewMesh->InitResources();
				NewMesh->SetLightingGuid();

				FStaticMeshSourceModel& SrcModel = NewMesh->AddSourceModel();
				SrcModel.BuildSettings.bRecomputeNormals = true;
				SrcModel.BuildSettings.bRecomputeTangents = true;
				SrcModel.BuildSettings.bRemoveDegenerates = false;
				SrcModel.BuildSettings.bGenerateLightmapUVs = false;

				FMeshDescription OutMeshDesc;
				FStaticMeshAttributes(OutMeshDesc).Register();
				FDynamicMeshToMeshDescription MeshConverter;
				MeshConverter.Convert(&Result.ResultMesh, OutMeshDesc);

				NewMesh->CreateMeshDescription(0, MoveTemp(OutMeshDesc));
				NewMesh->CommitMeshDescription(0);
				NewMesh->Build(false);
				NewMesh->PostEditChange();

				Self->TempCollisionMesh = NewMesh;
				Self->AddManagedObject(NewMesh);
			}

			// Update collision actor in preview scene
			Self->UpdateCollisionActor(Self->TempCollisionMesh);

			// Set as complex collision on source mesh
			if (Self->SourceMesh->GetBodySetup())
			{
				Self->SourceMesh->GetBodySetup()->Modify();
				Self->SourceMesh->ComplexCollisionMesh = Self->TempCollisionMesh;
				Self->SourceMesh->GetBodySetup()->InvalidatePhysicsData();
				Self->SourceMesh->GetBodySetup()->CreatePhysicsMeshes();
			}

			Self->UpdateStatsOverlay();

			// Build response JSON
			auto ResultJson = MakeShared<FJsonObject>();
			ResultJson->SetNumberField(TEXT("outputVertexCount"), Result.OutputVertexCount);
			ResultJson->SetNumberField(TEXT("outputTriangleCount"), Result.OutputTriangleCount);
			ResultJson->SetNumberField(TEXT("inputVertexCount"), Result.InputVertexCount);
			ResultJson->SetNumberField(TEXT("inputTriangleCount"), Result.InputTriangleCount);
			ResultJson->SetNumberField(TEXT("elapsedSeconds"), Result.ElapsedSeconds);
			if (!Result.WarningMessage.IsEmpty())
			{
				ResultJson->SetStringField(TEXT("warning"), Result.WarningMessage);
			}
			double Ratio = Result.InputTriangleCount > 0
				? (double)Result.OutputTriangleCount / Result.InputTriangleCount
				: 0.0;
			ResultJson->SetNumberField(TEXT("triangleRatio"), Ratio);

			Captured.Complete(ResultJson);
		});
	});

	return Deferred;
}

// ============================================================================
// UpdateCollisionActor
// ============================================================================

void UPneumaViewCollision::UpdateCollisionActor(UStaticMesh* NewCollisionMesh)
{
	if (!CollisionActor || !NewCollisionMesh) return;
	SetActorStaticMesh(CollisionActor, NewCollisionMesh);
}

// ============================================================================
// UpdateStatsOverlay
// ============================================================================

void UPneumaViewCollision::UpdateStatsOverlay()
{
	UPneumaViewConfig_Collision* CollisionConfig = Cast<UPneumaViewConfig_Collision>(Config);
	if (CollisionConfig && CollisionConfig->StatsOverlay.IsValid())
	{
		CollisionConfig->StatsOverlay->RefreshStats();
	}
}

// ============================================================================
// Inspect
// ============================================================================

FString UPneumaViewCollision::Inspect()
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	// Source stats
	auto SourceJson = MakeShared<FJsonObject>();
	if (SourceMesh && SourceMesh->GetRenderData() && SourceMesh->GetRenderData()->LODResources.Num() > 0)
	{
		const auto& LOD0 = SourceMesh->GetRenderData()->LODResources[0];
		SourceJson->SetNumberField(TEXT("vertexCount"), LOD0.GetNumVertices());
		SourceJson->SetNumberField(TEXT("triangleCount"), LOD0.GetNumTriangles());
		FVector Size = SourceMesh->GetBoundingBox().GetSize() / 100.0;
		auto SizeJson = MakeShared<FJsonObject>();
		SizeJson->SetNumberField(TEXT("X"), Size.X);
		SizeJson->SetNumberField(TEXT("Y"), Size.Y);
		SizeJson->SetNumberField(TEXT("Z"), Size.Z);
		SourceJson->SetObjectField(TEXT("boundsSize"), SizeJson);
	}
	Root->SetObjectField(TEXT("source"), SourceJson);

	// Collision stats
	auto CollisionJson = MakeShared<FJsonObject>();
	CollisionJson->SetBoolField(TEXT("isGenerated"), TempCollisionMesh != nullptr);
	if (TempCollisionMesh && TempCollisionMesh->GetRenderData() && TempCollisionMesh->GetRenderData()->LODResources.Num() > 0)
	{
		const auto& LOD0 = TempCollisionMesh->GetRenderData()->LODResources[0];
		int32 ColTris = LOD0.GetNumTriangles();
		CollisionJson->SetNumberField(TEXT("vertexCount"), LOD0.GetNumVertices());
		CollisionJson->SetNumberField(TEXT("triangleCount"), ColTris);
		FVector Size = TempCollisionMesh->GetBoundingBox().GetSize() / 100.0;
		auto SizeJson = MakeShared<FJsonObject>();
		SizeJson->SetNumberField(TEXT("X"), Size.X);
		SizeJson->SetNumberField(TEXT("Y"), Size.Y);
		SizeJson->SetNumberField(TEXT("Z"), Size.Z);
		CollisionJson->SetObjectField(TEXT("boundsSize"), SizeJson);

		if (SourceMesh && SourceMesh->GetRenderData() && SourceMesh->GetRenderData()->LODResources.Num() > 0)
		{
			int32 SrcTris = SourceMesh->GetRenderData()->LODResources[0].GetNumTriangles();
			if (SrcTris > 0)
			{
				CollisionJson->SetNumberField(TEXT("triangleRatio"), (double)ColTris / SrcTris);
			}
		}
	}
	Root->SetObjectField(TEXT("collision"), CollisionJson);

	// Multi-angle captures
	TArray<FPneumaCameraPose> Poses = ComputeInspectCameraPoses();
	static const TArray<FString> AngleNames = {
		TEXT("front"), TEXT("back"), TEXT("left"), TEXT("right"), TEXT("top"), TEXT("perspective")
	};

	UWorld* PreviewWorld = GetPreviewWorldPtr();
	TArray<TSharedPtr<FJsonValue>> CapturesArray;

	if (PreviewWorld && Poses.Num() > 0)
	{
		const int32 CaptureW = 512;
		const int32 CaptureH = 512;

		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		RT->InitAutoFormat(CaptureW, CaptureH);
		RT->UpdateResourceImmediate(true);

		ASceneCapture2D* CaptureActor = PreviewWorld->SpawnActor<ASceneCapture2D>();
		CaptureActor->SetFlags(RF_Transient);
		USceneCaptureComponent2D* CaptureComp = CaptureActor->GetCaptureComponent2D();
		CaptureComp->TextureTarget = RT;
		CaptureComp->CaptureSource = SCS_FinalColorLDR;
		CaptureComp->bCaptureEveryFrame = false;
		CaptureComp->bCaptureOnMovement = false;
		CaptureComp->bAlwaysPersistRenderingState = true;

		FString OutputDir = FPaths::ProjectSavedDir() / TEXT("PneumaView") / UniqueName;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*OutputDir))
		{
			PlatformFile.CreateDirectoryTree(*OutputDir);
		}

		FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();

		for (int32 i = 0; i < Poses.Num() && i < AngleNames.Num(); ++i)
		{
			CaptureComp->SetWorldLocationAndRotation(Poses[i].Location, Poses[i].Rotation);
			CaptureComp->CaptureScene();

			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm, CubeFace_MAX);
			RTResource->ReadPixels(Pixels, ReadFlags, FIntRect(0, 0, CaptureW, CaptureH));

			FString FilePath = OutputDir / FString::Printf(TEXT("%s.png"), *AngleNames[i]);
			FImageView ImageView(Pixels.GetData(), CaptureW, CaptureH);
			FImageUtils::SaveImageByExtension(*FilePath, ImageView);

			auto CaptureJson = MakeShared<FJsonObject>();
			CaptureJson->SetStringField(TEXT("angle"), AngleNames[i]);
			CaptureJson->SetStringField(TEXT("path"), FilePath);
			CapturesArray.Add(MakeShared<FJsonValueObject>(CaptureJson));
		}

		CaptureActor->Destroy();
		RT->MarkAsGarbage();
	}

	Root->SetArrayField(TEXT("captures"), CapturesArray);
	Root->SetNumberField(TEXT("lastGenerateElapsed"), LastGenElapsed);

	return CollisionJsonToString(Root);
}

// ============================================================================
// ComputeInspectCameraPoses
// ============================================================================

TArray<FPneumaCameraPose> UPneumaViewCollision::ComputeInspectCameraPoses() const
{
	TArray<FPneumaCameraPose> Poses;

	FBox CombinedBounds(ForceInit);
	for (const TObjectPtr<UObject>& Obj : ManagedObjects)
	{
		if (AActor* Actor = Cast<AActor>(Obj.Get()))
		{
			if (IsValid(Actor))
			{
				CombinedBounds += Actor->GetComponentsBoundingBox(true);
			}
		}
	}

	if (!CombinedBounds.IsValid) return Poses;

	FVector Center = CombinedBounds.GetCenter();
	double Distance = CombinedBounds.GetExtent().Size() * 2.4;

	struct FAngleDef { FString Name; FVector Dir; };
	TArray<FAngleDef> Angles = {
		{ TEXT("front"),       FVector(1, 0, 0) },
		{ TEXT("back"),        FVector(-1, 0, 0) },
		{ TEXT("left"),        FVector(0, 1, 0) },
		{ TEXT("right"),       FVector(0, -1, 0) },
		{ TEXT("top"),         FVector(0, 0, 1) },
		{ TEXT("perspective"), FVector(1, 1, 1).GetSafeNormal() },
	};

	for (const FAngleDef& Angle : Angles)
	{
		FVector Pos = Center + Angle.Dir * Distance;
		FRotator Rot = (Center - Pos).Rotation();

		FPneumaCameraPose Pose;
		Pose.Location = Pos;
		Pose.Rotation = Rot;
		Poses.Add(Pose);
	}

	return Poses;
}

// ============================================================================
// Accept / Reject
// ============================================================================

bool UPneumaViewCollision::Accept()
{
	if (!SourceMesh || !TempCollisionMesh)
	{
		UE_LOG(LogCollisionGen, Error, TEXT("Accept: no collision mesh generated yet"));
		return false;
	}

	// ComplexCollisionMesh is already set during Generate, just mark the source mesh dirty
	SourceMesh->MarkPackageDirty();

	UE_LOG(LogCollisionGen, Log, TEXT("Accept: collision mesh applied to %s"), *SourceMesh->GetPathName());

	Close();
	return true;
}

bool UPneumaViewCollision::Reject()
{
	CleanupTempAssets();
	Close();
	return true;
}

void UPneumaViewCollision::CleanupTempAssets()
{
	if (SourceMesh && SourceMesh->GetBodySetup())
	{
		SourceMesh->ComplexCollisionMesh = nullptr;
	}

	if (TempCollisionMesh)
	{
		RemoveManagedObject(TempCollisionMesh);
		TempCollisionMesh->MarkAsGarbage();
		TempCollisionMesh = nullptr;
	}
}

void UPneumaViewCollision::OnEditorClosed()
{
	CleanupTempAssets();
	Super::OnEditorClosed();
}

