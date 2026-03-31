// MIT License - Copyright (c) 2025 Italink
// Ported from VirtualCollider CollisionMeshGenerator_Approximate

#include "CollisionMeshAlgorithm.h"
#include "Collision/PneumaViewCollision.h"

#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "Implicit/Solidify.h"
#include "DynamicMesh/ColliderMesh.h"
#include "MeshSimplification.h"
#include "MeshBoundaryLoops.h"
#include "Operations/MinimalHoleFiller.h"
#include "MeshQueries.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// ---- OpenVDB includes ----
#if PLATFORM_WINDOWS
#include "HAL/Platform.h"
#include "HAL/PlatformAtomics.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
THIRD_PARTY_INCLUDES_START
UE_PUSH_MACRO("check")
#undef check
#pragma warning(push)
#pragma warning(disable: 4146)
#pragma warning(disable: 4541)
#pragma warning(disable: 4706)
#ifndef M_PI
	#define M_PI 3.14159265358979323846
	#define LOCAL_M_PI 1
#endif
#ifndef M_PI_2
	#define M_PI_2 1.57079632679489661923
	#define LOCAL_M_PI_2 1
#endif
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/LevelSetFilter.h>
#include <openvdb/tools/LevelSetRebuild.h>
#undef GetObject
#ifdef LOCAL_M_PI
	#undef M_PI
#endif
#ifdef LOCAL_M_PI_2
	#undef M_PI_2
#endif
#pragma warning(pop)
UE_POP_MACRO("check")
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#define HAS_OPENVDB 1
#else
#define HAS_OPENVDB 0
#endif

using namespace UE::Geometry;

DEFINE_LOG_CATEGORY_STATIC(LogCollisionAlgo, Log, All);

// ============================================================================
// OpenVDB mesh adapter
// ============================================================================

#if HAS_OPENVDB
class FDynamicMeshVDBAdapter
{
public:
	FDynamicMeshVDBAdapter(const FDynamicMesh3& InMesh, const openvdb::math::Transform& InTransform)
		: Mesh(InMesh), Transform(InTransform)
	{
		TriangleIDs.Reserve(Mesh.TriangleCount());
		for (int32 TID : Mesh.TriangleIndicesItr())
		{
			TriangleIDs.Add(TID);
		}
	}

	size_t polygonCount() const { return (size_t)TriangleIDs.Num(); }
	size_t pointCount() const { return (size_t)Mesh.MaxVertexID(); }
	size_t vertexCount(size_t) const { return 3; }

	void getIndexSpacePoint(size_t FaceNumber, size_t CornerNumber, openvdb::Vec3d& pos) const
	{
		const FIndex3i Tri = Mesh.GetTriangle(TriangleIDs[(int32)FaceNumber]);
		const int32 VID = Tri[static_cast<int>(CornerNumber)];
		const FVector3d P = Mesh.GetVertex(VID);
		pos = Transform.worldToIndex(openvdb::Vec3d(P.X, P.Y, P.Z));
	}

private:
	const FDynamicMesh3& Mesh;
	const openvdb::math::Transform& Transform;
	TArray<int32> TriangleIDs;
};

static bool DynamicMeshToSDF(const FDynamicMesh3& Mesh, double VoxelSize, float HalfBandWidth,
	openvdb::FloatGrid::Ptr& OutSDFGrid)
{
	openvdb::math::Transform::Ptr XForm = openvdb::math::Transform::createLinearTransform(VoxelSize);
	FDynamicMeshVDBAdapter Adapter(Mesh, *XForm);

	try
	{
		OutSDFGrid = openvdb::tools::meshToVolume<openvdb::FloatGrid>(Adapter, *XForm, HalfBandWidth, HalfBandWidth);
		openvdb::tools::pruneLevelSet(OutSDFGrid->tree(), HalfBandWidth, -HalfBandWidth);
	}
	catch (std::bad_alloc&)
	{
		UE_LOG(LogCollisionAlgo, Error, TEXT("DynamicMeshToSDF: out of memory"));
		if (OutSDFGrid) OutSDFGrid->tree().clear();
		return false;
	}
	return true;
}

static void SDFToDynamicMesh(const openvdb::FloatGrid::Ptr& SDFGrid, double IsoValue, FDynamicMesh3& OutMesh)
{
	std::vector<openvdb::Vec3s> Points;
	std::vector<openvdb::Vec3I> Triangles;
	std::vector<openvdb::Vec4I> Quads;

	openvdb::tools::volumeToMesh(*SDFGrid, Points, Triangles, Quads, IsoValue, 0.0);

	OutMesh.Clear();
	for (const openvdb::Vec3s& P : Points)
		OutMesh.AppendVertex(FVector3d((double)P.x(), (double)P.y(), (double)P.z()));
	for (const openvdb::Vec3I& T : Triangles)
		OutMesh.AppendTriangle((int32)T.x(), (int32)T.y(), (int32)T.z());
	for (const openvdb::Vec4I& Q : Quads)
	{
		OutMesh.AppendTriangle((int32)Q.x(), (int32)Q.y(), (int32)Q.z());
		OutMesh.AppendTriangle((int32)Q.x(), (int32)Q.z(), (int32)Q.w());
	}
}

static void CloseGapsSDF(openvdb::FloatGrid::Ptr& InOutSDFGrid, double GapRadius)
{
	if (!InOutSDFGrid) return;
	const double InputVoxelSize = InOutSDFGrid->transform().voxelSize()[0];
	if (GapRadius < InputVoxelSize) return;

	const float HalfBandWidth = 3.0f;
	openvdb::FloatGrid::Ptr OriginalSDF = InOutSDFGrid->deepCopy();

	{
		openvdb::tools::LevelSetFilter<openvdb::FloatGrid> Filter(*InOutSDFGrid);
		Filter.offset(-static_cast<float>(GapRadius));
	}

	openvdb::FloatGrid::Ptr DilatedSDF = openvdb::tools::levelSetRebuild(*InOutSDFGrid, 0.0f, HalfBandWidth);

	{
		const float ErodeAmount = static_cast<float>(GapRadius + 0.5 * InputVoxelSize);
		openvdb::tools::LevelSetFilter<openvdb::FloatGrid> Filter(*DilatedSDF);
		Filter.offset(ErodeAmount);
	}

	openvdb::FloatGrid::Ptr ErodedSDF = openvdb::tools::levelSetRebuild(*DilatedSDF, 0.0f, HalfBandWidth);
	DilatedSDF.reset();

	openvdb::tools::csgUnion(*OriginalSDF, *ErodedSDF);
	ErodedSDF.reset();

	openvdb::tools::pruneLevelSet(OriginalSDF->tree(), HalfBandWidth, -HalfBandWidth);
	InOutSDFGrid = OriginalSDF;
}
#endif // HAS_OPENVDB

// ============================================================================
// Estimate boundary loop area (Newell's method)
// ============================================================================

static double EstimateLoopArea(const FDynamicMesh3& Mesh, const FEdgeLoop& Loop)
{
	if (Loop.Vertices.Num() < 3) return 0.0;
	FVector3d Normal = FVector3d::Zero();
	int N = Loop.Vertices.Num();
	for (int i = 0; i < N; i++)
	{
		const FVector3d& Curr = Mesh.GetVertex(Loop.Vertices[i]);
		const FVector3d& Next = Mesh.GetVertex(Loop.Vertices[(i + 1) % N]);
		Normal.X += (Curr.Y - Next.Y) * (Curr.Z + Next.Z);
		Normal.Y += (Curr.Z - Next.Z) * (Curr.X + Next.X);
		Normal.Z += (Curr.X - Next.X) * (Curr.Y + Next.Y);
	}
	return 0.5 * Normal.Length();
}

// ============================================================================
// Generate
// ============================================================================

FCollisionMeshResult FCollisionMeshAlgorithm::Generate(
	const FDynamicMesh3& InSourceMesh,
	const FCollisionGenParams& Params)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CollisionMeshAlgorithm::Generate);

	FCollisionMeshResult Result;
	double StartTime = FPlatformTime::Seconds();

	FDynamicMesh3 WorkMesh(InSourceMesh);
	WorkMesh.DiscardAttributes();

	Result.InputVertexCount = WorkMesh.VertexCount();
	Result.InputTriangleCount = WorkMesh.TriangleCount();

	if (WorkMesh.TriangleCount() == 0)
	{
		Result.WarningMessage = TEXT("Empty input mesh");
		return Result;
	}

	const double MetersToUE = 100.0;

	// Compute voxel size
	double VoxelSizeUE;
	if (Params.VoxelSizeMode == ECollisionVoxelSizeMode::Resolution)
	{
		FAxisAlignedBox3d MeshBounds = WorkMesh.GetBounds();
		VoxelSizeUE = MeshBounds.MaxDim() / FMath::Max(Params.VoxelResolution, 8);
	}
	else
	{
		VoxelSizeUE = Params.VoxelSize * MetersToUE;
	}

	const double GeometricDeviationUE = Params.GeometricDeviation * MetersToUE;
	const double MaxHoleAreaUE = Params.MaxHoleArea * MetersToUE * MetersToUE;

	// Memory protection
	{
		FAxisAlignedBox3d Bounds = WorkMesh.GetBounds();
		double EstVoxels = (Bounds.Width() / VoxelSizeUE) * (Bounds.Height() / VoxelSizeUE) * (Bounds.Depth() / VoxelSizeUE);
		if (EstVoxels > 5e8)
		{
			double Scale = FMath::Pow(5e8 / EstVoxels, 1.0 / 3.0);
			VoxelSizeUE /= Scale;
			Result.WarningMessage = FString::Printf(TEXT("Voxel count too high (%.0f), auto-reduced resolution"), EstVoxels);
			UE_LOG(LogCollisionAlgo, Warning, TEXT("%s"), *Result.WarningMessage);
		}
	}

	// Step 1: Weld boundary edges
	if (Params.bWeldBoundaryEdges)
	{
		const double WeldToleranceUE = Params.WeldTolerance * MetersToUE;
		FMergeCoincidentMeshEdges Merger(&WorkMesh);
		Merger.MergeVertexTolerance = WeldToleranceUE;
		Merger.MergeSearchTolerance = WeldToleranceUE * 2.0;
		Merger.OnlyUniquePairs = false;
		Merger.Apply();
	}

	// Step 2: Fill small holes
	if (Params.bFillSmallHoles && MaxHoleAreaUE > 0)
	{
		FMeshBoundaryLoops BoundaryLoops(&WorkMesh, true);
		for (FEdgeLoop& Loop : BoundaryLoops.Loops)
		{
			if (EstimateLoopArea(WorkMesh, Loop) <= MaxHoleAreaUE)
			{
				FMinimalHoleFiller Filler(&WorkMesh, Loop);
				Filler.Fill();
			}
		}
	}

	// Step 3: Solidification
	FDynamicMeshAABBTree3 SrcSpatial;
	TUniquePtr<TFastWindingTree<FDynamicMesh3>> Winding;
	{
		SrcSpatial.SetMesh(&WorkMesh, true);
		Winding = MakeUnique<TFastWindingTree<FDynamicMesh3>>(&SrcSpatial, true);
	}

	const double ShellThicknessUE = Params.bThickenThinParts
		? (Params.bOverrideShellThickness ? (double)Params.ShellThickness * MetersToUE : VoxelSizeUE * 1.5)
		: 0.0;

	FDynamicMesh3 SolidMesh;
	if (Params.bThickenThinParts && ShellThicknessUE > 0)
	{
		const double ShellDistSqr = ShellThicknessUE * ShellThicknessUE;
		FAxisAlignedBox3d MeshBounds = WorkMesh.GetBounds();

		TArray<FVector3d> SeedPoints;
		SeedPoints.Reserve(WorkMesh.VertexCount());
		for (int32 vid : WorkMesh.VertexIndicesItr())
			SeedPoints.Add(WorkMesh.GetVertex(vid));

		float WindingThreshold = Params.WindingThreshold;
		auto ShellWindingFunction = [&SrcSpatial, &Winding, ShellDistSqr, WindingThreshold](const FVector3d& Pos) -> double
		{
			double W = Winding->FastWindingNumber(Pos);
			if (W >= WindingThreshold) return W;

			double NearestDistSqr = ShellDistSqr;
			int32 NearTriID = SrcSpatial.FindNearestTriangle(Pos, NearestDistSqr,
				IMeshSpatial::FQueryOptions(FMath::Sqrt(ShellDistSqr)));
			if (NearTriID != IndexConstants::InvalidID && NearestDistSqr < ShellDistSqr)
				return 1.0;
			return W;
		};

		FWindingNumberBasedSolidify Solidify(
			TUniqueFunction<double(const FVector3d&)>(MoveTemp(ShellWindingFunction)),
			MeshBounds, SeedPoints);
		Solidify.SetCellSizeAndExtendBounds(MeshBounds, VoxelSizeUE * 2.0,
			FMath::Clamp((int)(MeshBounds.MaxDim() / VoxelSizeUE), 16, 4096));
		Solidify.WindingThreshold = Params.WindingThreshold;
		SolidMesh = FDynamicMesh3(&Solidify.Generate());
	}
	else
	{
		FAxisAlignedBox3d MeshBounds = WorkMesh.GetBounds();
		TImplicitSolidify<FDynamicMesh3> Solidify(&WorkMesh, &SrcSpatial, Winding.Get());
		Solidify.SetCellSizeAndExtendBounds(MeshBounds, VoxelSizeUE * 2.0,
			FMath::Clamp((int)(MeshBounds.MaxDim() / VoxelSizeUE), 16, 4096));
		Solidify.WindingThreshold = Params.WindingThreshold;
		SolidMesh = FDynamicMesh3(&Solidify.Generate());
	}

	if (SolidMesh.TriangleCount() == 0)
	{
		Result.WarningMessage = TEXT("Solidification produced empty mesh");
		Result.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
		return Result;
	}

	// Step 3b: Gap closing via OpenVDB
#if HAS_OPENVDB
	if (Params.bCloseGaps)
	{
		const double GapClosingRadiusUE = (double)Params.GapClosingRadius * MetersToUE;
		if (GapClosingRadiusUE >= VoxelSizeUE)
		{
			openvdb::initialize();

			const float HalfBandWidth = 3.0f;
			openvdb::FloatGrid::Ptr SDFGrid;

			if (DynamicMeshToSDF(SolidMesh, VoxelSizeUE, HalfBandWidth, SDFGrid))
			{
				CloseGapsSDF(SDFGrid, GapClosingRadiusUE);
				SDFToDynamicMesh(SDFGrid, 0.0, SolidMesh);
				SDFGrid.reset();

				if (SolidMesh.TriangleCount() == 0)
				{
					Result.WarningMessage = TEXT("Gap closing produced empty mesh");
					Result.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;
					return Result;
				}
			}
		}
	}
#endif

	// Step 4: Project to source
	if (Params.bProjectToSource && Params.ProjectionBlendWeight > 0)
	{
		const double MaxProjectDistSqr = VoxelSizeUE * VoxelSizeUE * 9.0;
		for (int vid : SolidMesh.VertexIndicesItr())
		{
			FVector3d Pos = SolidMesh.GetVertex(vid);
			double NearestDistSqr = MaxProjectDistSqr;
			int NearTriID = SrcSpatial.FindNearestTriangle(Pos, NearestDistSqr);
			if (NearTriID >= 0 && NearestDistSqr < MaxProjectDistSqr)
			{
				FDistPoint3Triangle3d Query = TMeshQueries<FDynamicMesh3>::TriangleDistance(WorkMesh, NearTriID, Pos);
				FVector3d Projected = Query.ClosestTrianglePoint;
				SolidMesh.SetVertex(vid,
					(1.0 - Params.ProjectionBlendWeight) * Pos + Params.ProjectionBlendWeight * Projected);
			}
		}
	}

	// Step 5: Simplification
	if (GeometricDeviationUE > 0)
	{
		FVolPresMeshSimplification Simplifier(&SolidMesh);
		Simplifier.DEBUG_CHECK_LEVEL = 0;

		if (Params.bUseFastPath && SolidMesh.TriangleCount() > 1000000)
		{
			Simplifier.FastCollapsePass(VoxelSizeUE, 10, true, 1000000);
		}

		if (SolidMesh.TriangleCount() > 50000)
		{
			Simplifier.SimplifyToTriangleCount(50000);
		}

		{
			FColliderMesh SolidCollider;
			SolidCollider.Initialize(SolidMesh);
			FColliderMeshProjectionTarget SolidTarget(&SolidCollider);

			Simplifier.SetProjectionTarget(&SolidTarget);
			Simplifier.GeometricErrorConstraint =
				FVolPresMeshSimplification::EGeometricErrorCriteria::PredictedPointToProjectionTarget;
			Simplifier.GeometricErrorTolerance = GeometricDeviationUE;
			Simplifier.SimplifyToTriangleCount(8);
		}
	}

	Result.ResultMesh = MoveTemp(SolidMesh);
	Result.OutputVertexCount = Result.ResultMesh.VertexCount();
	Result.OutputTriangleCount = Result.ResultMesh.TriangleCount();
	Result.ElapsedSeconds = FPlatformTime::Seconds() - StartTime;

	UE_LOG(LogCollisionAlgo, Log, TEXT("Generate: %d -> %d tris in %.2fs"),
		Result.InputTriangleCount, Result.OutputTriangleCount, Result.ElapsedSeconds);

	return Result;
}

// ============================================================================
// AnalyzeMesh
// ============================================================================

static FString AlgoJsonToString(const TSharedRef<FJsonObject>& Obj)
{
	FString Out;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Obj, Writer);
	return Out;
}

FString FCollisionMeshAlgorithm::AnalyzeMesh(const FDynamicMesh3& Mesh)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();

	Result->SetNumberField(TEXT("vertexCount"), Mesh.VertexCount());
	Result->SetNumberField(TEXT("triangleCount"), Mesh.TriangleCount());

	// Bounds
	FAxisAlignedBox3d Bounds = Mesh.GetBounds();
	FVector3d Size = Bounds.Diagonal();
	auto SizeJson = MakeShared<FJsonObject>();
	SizeJson->SetNumberField(TEXT("X"), Size.X / 100.0);
	SizeJson->SetNumberField(TEXT("Y"), Size.Y / 100.0);
	SizeJson->SetNumberField(TEXT("Z"), Size.Z / 100.0);
	Result->SetObjectField(TEXT("boundsSize"), SizeJson);
	Result->SetNumberField(TEXT("maxDimension"), Bounds.MaxDim() / 100.0);

	// Boundary analysis
	int32 BoundaryEdgeCount = 0;
	for (int32 eid : Mesh.EdgeIndicesItr())
	{
		if (Mesh.IsBoundaryEdge(eid))
			BoundaryEdgeCount++;
	}
	Result->SetNumberField(TEXT("boundaryEdgeCount"), BoundaryEdgeCount);
	Result->SetBoolField(TEXT("isClosed"), BoundaryEdgeCount == 0);

	// Boundary loops
	FDynamicMesh3 TempMesh(Mesh);
	FMeshBoundaryLoops BoundaryLoops(&TempMesh, true);
	Result->SetNumberField(TEXT("boundaryLoopCount"), BoundaryLoops.Loops.Num());

	// Material count
	int32 MaxMaterialID = 0;
	if (Mesh.HasTriangleGroups())
	{
		for (int32 tid : Mesh.TriangleIndicesItr())
		{
			MaxMaterialID = FMath::Max(MaxMaterialID, Mesh.GetTriangleGroup(tid));
		}
	}
	Result->SetNumberField(TEXT("materialCount"), MaxMaterialID + 1);

	// Thin wall heuristic: high boundary-edge-to-total-edge ratio
	int32 TotalEdges = Mesh.EdgeCount();
	double ThinWallRatio = TotalEdges > 0 ? (double)BoundaryEdgeCount / TotalEdges : 0.0;
	Result->SetBoolField(TEXT("hasThinWalls"), ThinWallRatio > 0.05);
	Result->SetNumberField(TEXT("thinWallRatio"), ThinWallRatio);

	// Suggested resolution
	double MaxDimMeters = Bounds.MaxDim() / 100.0;
	int32 SuggestedRes = FMath::Clamp((int32)(MaxDimMeters * 100.0), 64, 512);
	Result->SetNumberField(TEXT("suggestedVoxelResolution"), SuggestedRes);

	// Suggested params
	auto Suggested = MakeShared<FJsonObject>();
	Suggested->SetBoolField(TEXT("bWeldBoundaryEdges"), BoundaryEdgeCount > 0);
	Suggested->SetBoolField(TEXT("bFillSmallHoles"), BoundaryLoops.Loops.Num() > 3);
	Suggested->SetBoolField(TEXT("bThickenThinParts"), ThinWallRatio > 0.05);
	Suggested->SetBoolField(TEXT("bCloseGaps"), false);
	Suggested->SetBoolField(TEXT("bProjectToSource"), true);
	Result->SetObjectField(TEXT("suggestedParams"), Suggested);

	return AlgoJsonToString(Result);
}
