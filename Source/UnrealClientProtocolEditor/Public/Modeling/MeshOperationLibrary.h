// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "GeometryScript/GeometryScriptSelectionTypes.h"
#include "MeshOperationLibrary.generated.h"

class UDynamicMesh;
class UStaticMesh;

UCLASS()
class UMeshOperationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ── DynamicMesh Lifecycle ──

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static UDynamicMesh* CreateDynamicMesh();

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static bool ReleaseDynamicMesh(UDynamicMesh* Mesh);

	// ── Mesh Summary ──

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static FString GetMeshSummary(UDynamicMesh* TargetMesh);

	// ── Simplified Asset I/O ──

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static UDynamicMesh* CopyFromStaticMesh(UDynamicMesh* TargetMesh, UStaticMesh* SourceStaticMesh);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static bool CopyToStaticMesh(UDynamicMesh* SourceMesh, UStaticMesh* TargetStaticMesh);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling")
	static FString CopyToNewStaticMeshAsset(UDynamicMesh* SourceMesh, const FString& AssetPath);

	// ── Selection Bridge ──
	// FGeometryScriptMeshSelection has non-UPROPERTY TSharedPtr storage
	// that cannot be serialized through UCP JSON. These functions bridge that gap.

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static FGeometryScriptMeshSelection SelectAllTriangles(UDynamicMesh* TargetMesh);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static FGeometryScriptMeshSelection SelectTrianglesByIDs(UDynamicMesh* TargetMesh, const TArray<int32>& TriangleIDs);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static FGeometryScriptMeshSelection SelectVerticesByIDs(UDynamicMesh* TargetMesh, const TArray<int32>& VertexIDs);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static FGeometryScriptMeshSelection SelectPolygroupByID(UDynamicMesh* TargetMesh, int32 PolygroupID, FGeometryScriptGroupLayer GroupLayer);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static FGeometryScriptMeshSelection SelectMaterialByID(UDynamicMesh* TargetMesh, int32 MaterialID);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Selection")
	static TArray<int32> GetSelectionIDs(FGeometryScriptMeshSelection Selection);

	// ── List/Path/Polygon Bridge ──
	// These GeometryScript types store data in non-UPROPERTY TSharedPtr<TArray<...>>
	// that cannot be serialized through UCP JSON.

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptIndexList MakeIndexList(const TArray<int32>& Indices, EGeometryScriptIndexType IndexType);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<int32> GetIndexListArray(FGeometryScriptIndexList IndexList);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptVectorList MakeVectorList(const TArray<FVector>& Vectors);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FVector> GetVectorListArray(FGeometryScriptVectorList VectorList);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptPolyPath MakePolyPath(const TArray<FVector>& Points, bool bClosedLoop);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FVector> GetPolyPathArray(FGeometryScriptPolyPath PolyPath);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptSimplePolygon MakeSimplePolygon(const TArray<FVector2D>& Vertices);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FVector2D> GetSimplePolygonArray(FGeometryScriptSimplePolygon Polygon);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptColorList MakeColorList(const TArray<FLinearColor>& Colors);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FLinearColor> GetColorListArray(FGeometryScriptColorList ColorList);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptUVList MakeUVList(const TArray<FVector2D>& UVs);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FVector2D> GetUVListArray(FGeometryScriptUVList UVList);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptScalarList MakeScalarList(const TArray<double>& Scalars);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<double> GetScalarListArray(FGeometryScriptScalarList ScalarList);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static FGeometryScriptTriangleList MakeTriangleList(const TArray<FIntVector>& Triangles);

	UFUNCTION(BlueprintCallable, Category = "UCP|Modeling|Data")
	static TArray<FIntVector> GetTriangleListArray(FGeometryScriptTriangleList TriangleList);

};
