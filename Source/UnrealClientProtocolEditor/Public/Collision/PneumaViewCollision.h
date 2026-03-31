// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "PneumaView/PneumaView.h"
#include "UCPDeferredResponse.h"
#include "PneumaViewCollision.generated.h"

UENUM(BlueprintType)
enum class ECollisionVoxelSizeMode : uint8
{
	FixedSize   UMETA(DisplayName = "Fixed Size (m)"),
	Resolution  UMETA(DisplayName = "Resolution"),
};

USTRUCT(BlueprintType)
struct FCollisionGenParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preprocessing")
	bool bWeldBoundaryEdges = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preprocessing")
	float WeldTolerance = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preprocessing")
	bool bFillSmallHoles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preprocessing")
	float MaxHoleArea = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thickening")
	bool bThickenThinParts = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thickening")
	bool bOverrideShellThickness = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thickening")
	float ShellThickness = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxelization")
	ECollisionVoxelSizeMode VoxelSizeMode = ECollisionVoxelSizeMode::Resolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxelization")
	float VoxelSize = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxelization")
	int32 VoxelResolution = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxelization")
	float WindingThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GapClosing")
	bool bCloseGaps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GapClosing")
	float GapClosingRadius = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceProjection")
	bool bProjectToSource = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceProjection")
	float ProjectionBlendWeight = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simplification")
	bool bUseFastPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simplification")
	float GeometricDeviation = 0.1f;
};

UCLASS(BlueprintType)
class UPneumaViewCollision : public UPneumaView
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	static UPneumaViewCollision* Open(UStaticMesh* SourceMesh);

	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	FString Analyze();

	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	FUCPDeferredResponse Generate(const FCollisionGenParams& Params);

	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	FString Inspect();

	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	bool Accept();

	UFUNCTION(BlueprintCallable, Category = "UCP|Collision")
	bool Reject();

	AActor* GetSourceActor() const { return SourceActor; }
	AActor* GetCollisionActor() const { return CollisionActor; }

private:
	UPROPERTY()
	TObjectPtr<UStaticMesh> SourceMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TempCollisionMesh;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TObjectPtr<AActor> CollisionActor;

	FString CachedAnalysis;
	FCollisionGenParams LastGenParams;
	double LastGenElapsed = 0.0;

	void SetupPreviewScene();
	void UpdateCollisionActor(UStaticMesh* NewCollisionMesh);
	void UpdateStatsOverlay();
	void CleanupTempAssets();

	virtual void OnEditorClosed() override;

	TArray<FPneumaCameraPose> ComputeInspectCameraPoses() const;
};
