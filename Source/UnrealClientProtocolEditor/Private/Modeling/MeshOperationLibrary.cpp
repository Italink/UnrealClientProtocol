// MIT License - Copyright (c) 2025 Italink

#include "Modeling/MeshOperationLibrary.h"

#include "UDynamicMesh.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "GeometryScript/MeshSelectionFunctions.h"
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogMeshOp, Log, All);

static FString JsonToString(const TSharedRef<FJsonObject>& Obj)
{
	FString Out;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Obj, Writer);
	return Out;
}

// ── DynamicMesh Lifecycle ──

UDynamicMesh* UMeshOperationLibrary::CreateDynamicMesh()
{
	UDynamicMesh* Mesh = NewObject<UDynamicMesh>(GetTransientPackage());
	if (Mesh)
	{
		Mesh->Reset();
	}
	return Mesh;
}

bool UMeshOperationLibrary::ReleaseDynamicMesh(UDynamicMesh* Mesh)
{
	if (!Mesh)
	{
		return false;
	}
	Mesh->MarkAsGarbage();
	return true;
}

// ── Mesh Summary ──

FString UMeshOperationLibrary::GetMeshSummary(UDynamicMesh* TargetMesh)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	if (!TargetMesh)
	{
		Result->SetStringField(TEXT("error"), TEXT("null mesh"));
		return JsonToString(Result);
	}

	Result->SetStringField(TEXT("path"), TargetMesh->GetPathName());

	bool bIsDense = UGeometryScriptLibrary_MeshQueryFunctions::GetIsDenseMesh(TargetMesh);
	int32 VertexCount = UGeometryScriptLibrary_MeshQueryFunctions::GetVertexCount(TargetMesh);
	int32 TriangleCount = UGeometryScriptLibrary_MeshQueryFunctions::GetNumTriangleIDs(TargetMesh);
	int32 UVSetCount = UGeometryScriptLibrary_MeshQueryFunctions::GetNumUVSets(TargetMesh);
	bool bHasNormals = UGeometryScriptLibrary_MeshQueryFunctions::GetHasTriangleNormals(TargetMesh);
	bool bHasColors = UGeometryScriptLibrary_MeshQueryFunctions::GetHasVertexColors(TargetMesh);
	bool bHasMaterialIDs = UGeometryScriptLibrary_MeshQueryFunctions::GetHasMaterialIDs(TargetMesh);
	bool bHasPolygroups = UGeometryScriptLibrary_MeshQueryFunctions::GetHasPolygroups(TargetMesh);
	bool bIsClosed = UGeometryScriptLibrary_MeshQueryFunctions::GetIsClosedMesh(TargetMesh);

	FBox BoundingBox = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(TargetMesh);

	Result->SetNumberField(TEXT("vertexCount"), VertexCount);
	Result->SetNumberField(TEXT("triangleCount"), TriangleCount);
	Result->SetNumberField(TEXT("uvSetCount"), UVSetCount);
	Result->SetBoolField(TEXT("isDense"), bIsDense);
	Result->SetBoolField(TEXT("isClosed"), bIsClosed);
	Result->SetBoolField(TEXT("hasNormals"), bHasNormals);
	Result->SetBoolField(TEXT("hasVertexColors"), bHasColors);
	Result->SetBoolField(TEXT("hasMaterialIDs"), bHasMaterialIDs);
	Result->SetBoolField(TEXT("hasPolygroups"), bHasPolygroups);

	auto BoundsObj = MakeShared<FJsonObject>();
	auto MinObj = MakeShared<FJsonObject>();
	MinObj->SetNumberField(TEXT("X"), BoundingBox.Min.X);
	MinObj->SetNumberField(TEXT("Y"), BoundingBox.Min.Y);
	MinObj->SetNumberField(TEXT("Z"), BoundingBox.Min.Z);
	auto MaxObj = MakeShared<FJsonObject>();
	MaxObj->SetNumberField(TEXT("X"), BoundingBox.Max.X);
	MaxObj->SetNumberField(TEXT("Y"), BoundingBox.Max.Y);
	MaxObj->SetNumberField(TEXT("Z"), BoundingBox.Max.Z);
	BoundsObj->SetObjectField(TEXT("min"), MinObj);
	BoundsObj->SetObjectField(TEXT("max"), MaxObj);
	Result->SetObjectField(TEXT("bounds"), BoundsObj);

	return JsonToString(Result);
}

// ── Simplified Asset I/O ──

UDynamicMesh* UMeshOperationLibrary::CopyFromStaticMesh(UDynamicMesh* TargetMesh, UStaticMesh* SourceStaticMesh)
{
	if (!TargetMesh || !SourceStaticMesh)
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyFromStaticMesh: null argument"));
		return TargetMesh;
	}

	FGeometryScriptCopyMeshFromAssetOptions AssetOptions;
	FGeometryScriptMeshReadLOD ReadLOD;
	EGeometryScriptOutcomePins Outcome;

	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMeshV2(
		SourceStaticMesh, TargetMesh, AssetOptions, ReadLOD, Outcome, true, nullptr);

	if (Outcome != EGeometryScriptOutcomePins::Success)
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyFromStaticMesh: failed for %s"), *SourceStaticMesh->GetPathName());
	}
	return TargetMesh;
}

bool UMeshOperationLibrary::CopyToStaticMesh(UDynamicMesh* SourceMesh, UStaticMesh* TargetStaticMesh)
{
	if (!SourceMesh || !TargetStaticMesh)
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyToStaticMesh: null argument"));
		return false;
	}

	FGeometryScriptCopyMeshToAssetOptions Options;
	Options.bEnableRecomputeNormals = true;
	Options.bEnableRecomputeTangents = true;
	FGeometryScriptMeshWriteLOD WriteLOD;
	EGeometryScriptOutcomePins Outcome;

	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshToStaticMesh(
		SourceMesh, TargetStaticMesh, Options, WriteLOD, Outcome, true, nullptr);

	if (Outcome != EGeometryScriptOutcomePins::Success)
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyToStaticMesh: failed for %s"), *TargetStaticMesh->GetPathName());
		return false;
	}
	return true;
}

FString UMeshOperationLibrary::CopyToNewStaticMeshAsset(UDynamicMesh* SourceMesh, const FString& AssetPath)
{
	if (!SourceMesh || AssetPath.IsEmpty())
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyToNewStaticMeshAsset: invalid arguments"));
		return FString();
	}

	FGeometryScriptCreateNewStaticMeshAssetOptions Options;
	EGeometryScriptOutcomePins Outcome;

	UStaticMesh* NewAsset = UGeometryScriptLibrary_CreateNewAssetFunctions::CreateNewStaticMeshAssetFromMesh(
		SourceMesh, AssetPath, Options, Outcome, nullptr);

	if (Outcome != EGeometryScriptOutcomePins::Success || !NewAsset)
	{
		UE_LOG(LogMeshOp, Error, TEXT("CopyToNewStaticMeshAsset: failed to create %s"), *AssetPath);
		return FString();
	}
	return NewAsset->GetPathName();
}

// ── Selection Bridge ──

FGeometryScriptMeshSelection UMeshOperationLibrary::SelectAllTriangles(UDynamicMesh* TargetMesh)
{
	FGeometryScriptMeshSelection Selection;
	if (TargetMesh)
	{
		UGeometryScriptLibrary_MeshSelectionFunctions::CreateSelectAllMeshSelection(TargetMesh, Selection);
	}
	return Selection;
}

FGeometryScriptMeshSelection UMeshOperationLibrary::SelectTrianglesByIDs(UDynamicMesh* TargetMesh, const TArray<int32>& TriangleIDs)
{
	FGeometryScriptMeshSelection Selection;
	if (TargetMesh)
	{
		FGeometryScriptIndexList IndexList;
		IndexList.Reset(EGeometryScriptIndexType::Triangle, TriangleIDs.Num());
		*IndexList.List = TriangleIDs;
		UGeometryScriptLibrary_MeshSelectionFunctions::ConvertIndexListToMeshSelection(
			TargetMesh, IndexList, EGeometryScriptMeshSelectionType::Triangles, Selection);
	}
	return Selection;
}

FGeometryScriptMeshSelection UMeshOperationLibrary::SelectVerticesByIDs(UDynamicMesh* TargetMesh, const TArray<int32>& VertexIDs)
{
	FGeometryScriptMeshSelection Selection;
	if (TargetMesh)
	{
		FGeometryScriptIndexList IndexList;
		IndexList.Reset(EGeometryScriptIndexType::Vertex, VertexIDs.Num());
		*IndexList.List = VertexIDs;
		UGeometryScriptLibrary_MeshSelectionFunctions::ConvertIndexListToMeshSelection(
			TargetMesh, IndexList, EGeometryScriptMeshSelectionType::Vertices, Selection);
	}
	return Selection;
}

FGeometryScriptMeshSelection UMeshOperationLibrary::SelectPolygroupByID(UDynamicMesh* TargetMesh, int32 PolygroupID, FGeometryScriptGroupLayer GroupLayer)
{
	FGeometryScriptMeshSelection Selection;
	if (TargetMesh)
	{
		FGeometryScriptIndexList IndexList;
		IndexList.Reset(EGeometryScriptIndexType::PolygroupID, 1);
		IndexList.List->Add(PolygroupID);
		UGeometryScriptLibrary_MeshSelectionFunctions::ConvertIndexListToMeshSelection(
			TargetMesh, IndexList, EGeometryScriptMeshSelectionType::Polygroups, Selection);
	}
	return Selection;
}

FGeometryScriptMeshSelection UMeshOperationLibrary::SelectMaterialByID(UDynamicMesh* TargetMesh, int32 MaterialID)
{
	FGeometryScriptMeshSelection Selection;
	if (TargetMesh)
	{
		UGeometryScriptLibrary_MeshSelectionFunctions::SelectMeshElementsByMaterialID(
			TargetMesh, MaterialID, Selection);
	}
	return Selection;
}

TArray<int32> UMeshOperationLibrary::GetSelectionIDs(FGeometryScriptMeshSelection Selection)
{
	FGeometryScriptIndexList IndexList;
	EGeometryScriptIndexType ResultListType;
	UGeometryScriptLibrary_MeshSelectionFunctions::ConvertMeshSelectionToIndexList(
		nullptr, Selection, IndexList, ResultListType);
	if (IndexList.List.IsValid())
	{
		return *IndexList.List;
	}
	return {};
}

// ── List/Path/Polygon Bridge ──

FGeometryScriptIndexList UMeshOperationLibrary::MakeIndexList(const TArray<int32>& Indices, EGeometryScriptIndexType IndexType)
{
	FGeometryScriptIndexList Result;
	Result.Reset(IndexType, Indices.Num());
	*Result.List = Indices;
	return Result;
}

TArray<int32> UMeshOperationLibrary::GetIndexListArray(FGeometryScriptIndexList IndexList)
{
	return IndexList.List.IsValid() ? *IndexList.List : TArray<int32>();
}

FGeometryScriptVectorList UMeshOperationLibrary::MakeVectorList(const TArray<FVector>& Vectors)
{
	FGeometryScriptVectorList Result;
	Result.Reset(Vectors.Num());
	*Result.List = Vectors;
	return Result;
}

TArray<FVector> UMeshOperationLibrary::GetVectorListArray(FGeometryScriptVectorList VectorList)
{
	return VectorList.List.IsValid() ? *VectorList.List : TArray<FVector>();
}

FGeometryScriptPolyPath UMeshOperationLibrary::MakePolyPath(const TArray<FVector>& Points, bool bClosedLoop)
{
	FGeometryScriptPolyPath Result;
	Result.Reset(Points.Num());
	*Result.Path = Points;
	Result.bClosedLoop = bClosedLoop;
	return Result;
}

TArray<FVector> UMeshOperationLibrary::GetPolyPathArray(FGeometryScriptPolyPath PolyPath)
{
	return PolyPath.Path.IsValid() ? *PolyPath.Path : TArray<FVector>();
}

FGeometryScriptSimplePolygon UMeshOperationLibrary::MakeSimplePolygon(const TArray<FVector2D>& Vertices)
{
	FGeometryScriptSimplePolygon Result;
	Result.Reset(Vertices.Num());
	*Result.Vertices = Vertices;
	return Result;
}

TArray<FVector2D> UMeshOperationLibrary::GetSimplePolygonArray(FGeometryScriptSimplePolygon Polygon)
{
	return Polygon.Vertices.IsValid() ? *Polygon.Vertices : TArray<FVector2D>();
}

FGeometryScriptColorList UMeshOperationLibrary::MakeColorList(const TArray<FLinearColor>& Colors)
{
	FGeometryScriptColorList Result;
	Result.Reset(Colors.Num());
	*Result.List = Colors;
	return Result;
}

TArray<FLinearColor> UMeshOperationLibrary::GetColorListArray(FGeometryScriptColorList ColorList)
{
	return ColorList.List.IsValid() ? *ColorList.List : TArray<FLinearColor>();
}

FGeometryScriptUVList UMeshOperationLibrary::MakeUVList(const TArray<FVector2D>& UVs)
{
	FGeometryScriptUVList Result;
	Result.Reset(UVs.Num());
	*Result.List = UVs;
	return Result;
}

TArray<FVector2D> UMeshOperationLibrary::GetUVListArray(FGeometryScriptUVList UVList)
{
	return UVList.List.IsValid() ? *UVList.List : TArray<FVector2D>();
}

FGeometryScriptScalarList UMeshOperationLibrary::MakeScalarList(const TArray<double>& Scalars)
{
	FGeometryScriptScalarList Result;
	Result.Reset(Scalars.Num());
	*Result.List = Scalars;
	return Result;
}

TArray<double> UMeshOperationLibrary::GetScalarListArray(FGeometryScriptScalarList ScalarList)
{
	return ScalarList.List.IsValid() ? *ScalarList.List : TArray<double>();
}

FGeometryScriptTriangleList UMeshOperationLibrary::MakeTriangleList(const TArray<FIntVector>& Triangles)
{
	FGeometryScriptTriangleList Result;
	Result.Reset(Triangles.Num());
	*Result.List = Triangles;
	return Result;
}

TArray<FIntVector> UMeshOperationLibrary::GetTriangleListArray(FGeometryScriptTriangleList TriangleList)
{
	return TriangleList.List.IsValid() ? *TriangleList.List : TArray<FIntVector>();
}
