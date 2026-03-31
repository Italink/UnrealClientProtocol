// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"

struct FCollisionGenParams;

struct FCollisionMeshResult
{
	UE::Geometry::FDynamicMesh3 ResultMesh;
	int32 InputVertexCount = 0;
	int32 InputTriangleCount = 0;
	int32 OutputVertexCount = 0;
	int32 OutputTriangleCount = 0;
	double ElapsedSeconds = 0.0;
	FString WarningMessage;
};

class FCollisionMeshAlgorithm
{
public:
	static FCollisionMeshResult Generate(
		const UE::Geometry::FDynamicMesh3& SourceMesh,
		const FCollisionGenParams& Params);

	static FString AnalyzeMesh(
		const UE::Geometry::FDynamicMesh3& Mesh);
};
