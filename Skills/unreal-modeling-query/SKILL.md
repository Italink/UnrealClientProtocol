---
name: unreal-modeling-query
description: GeometryScript 网格查询与空间操作。当用户要求获取网格信息（顶点数、包围盒、拓扑状态）、空间查询（最近点、射线求交、包含测试）或采样时使用。
---

# 建模 — 查询与空间

前置条件：先阅读 `unreal-modeling`。

## 库

| 库 | CDO |
|----|-----|
| 查询 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshQueryFunctions` |
| 空间 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSpatialFunctions` |
| 采样 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSamplingFunctions` |
| 包含 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_ContainmentFunctions` |
| 比较 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshComparisonFunctions` |
| 测地线 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshGeodesicFunctions` |

提示：简单的 mesh 概览可直接用 `MeshOperationLibrary.GetMeshSummary`，一次调用返回所有基础信息。以下函数用于更精细的查询。

## 查询函数 — 全局信息

| 函数 | out 参数 | 说明 |
|------|----------|------|
| `GetMeshInfoString` | `InfoString` | 可读的 mesh 信息字符串 |
| `GetIsDenseMesh` | `bIsDense` | ID 空间是否无间隙 |
| `GetMeshBoundingBox` | 返回 `FBox` | 包围盒 |
| `GetMeshVolumeArea` | `SurfaceArea`, `Volume` | 表面积和体积 |
| `GetMeshVolumeAreaCenter` | `SurfaceArea`, `Volume`, `CenterOfMass` | 表面积、体积、质心 |
| `GetIsClosedMesh` | `bIsClosed` | 是否闭合 |
| `GetNumOpenBorderLoops` | `NumLoops` | 开放边界环数量 |
| `GetNumOpenBorderEdges` | `NumEdges` | 开放边界边数量 |
| `GetNumConnectedComponents` | `NumComponents` | 连通分量数 |
| `GetMeshHasAttributeSet` | `bHasAttributeSet` | 是否有属性集 |

## 查询函数 — 计数与 ID

| 函数 | out 参数 | 说明 |
|------|----------|------|
| `GetVertexCount` | 返回 int | 顶点数 |
| `GetNumVertexIDs` | `NumVertexIDs` | MaxVertexID |
| `GetHasVertexIDGaps` | `bHasGaps` | 顶点 ID 是否有间隙 |
| `GetNumTriangleIDs` | `NumTriangleIDs` | MaxTriangleID |
| `GetHasTriangleIDGaps` | `bHasGaps` | 三角形 ID 是否有间隙 |
| `GetNumUVSets` | `NumUVSets` | UV 通道数 |
| `GetNumExtendedPolygroupLayers` | 返回 int | 扩展 Polygroup 层数 |

## 查询函数 — 属性存在性

| 函数 | out 参数 |
|------|----------|
| `GetHasTriangleNormals` | `bHasNormals` |
| `GetHasVertexColors` | `bHasColors` |
| `GetHasMaterialIDs` | `bHasMaterialIDs` |
| `GetHasPolygroups` | `bHasPolygroups` |

## 查询函数 — 元素数据

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetVertexPosition` | `VertexID` | 返回顶点位置。out：`bIsValidVertex` |
| `GetTrianglePositions` | `TriangleID` | 返回三角形三个顶点位置。out：`bIsValidTriangle` |
| `GetTriangleIndices` | `TriangleID` | 返回三角形的三个顶点 ID |
| `GetTriangleFaceNormal` | `TriangleID` | 返回面法线 |
| `GetTriangleNormals` | `TriangleID` | 返回三个角的法线 |
| `GetTriangleUVs` | `TriangleID`, `UVSetIndex` | 返回三个角的 UV |
| `GetTriangleVertexColors` | `TriangleID` | 返回三个角的顶点色 |
| `GetVertexConnectedTriangles` | `VertexID` | 返回顶点关联的三角形 ID 列表 |
| `GetVertexConnectedVertices` | `VertexID` | 返回顶点的邻居顶点 ID 列表 |

## 查询函数 — 批量获取

| 函数 | 说明 |
|------|------|
| `GetAllTriangleIDs` | 返回所有三角形 ID 列表 |
| `GetAllTriangleIndices` | 返回所有三角形的顶点索引列表 |
| `GetAllVertexIDs` | 返回所有顶点 ID 列表 |
| `GetAllVertexPositions` | 返回所有顶点位置列表 |

注意：批量函数返回 `FGeometryScriptIndexList`、`FGeometryScriptTriangleList`、`FGeometryScriptVectorList` 等不可序列化类型。用 `MeshOperationLibrary` 的 `GetIndexListArray`、`GetTriangleListArray`、`GetVectorListArray` 提取数据。

## 空间函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `BuildBVHForMesh` | `TargetMesh` | 构建 BVH 加速结构。out：`OutputBVH` |
| `RebuildBVHForMesh` | `TargetMesh`, `UpdateBVH` | 重建已有 BVH |
| `FindNearestPointOnMesh` | `TargetMesh`, `QueryPoint`, `Options` | 查找 mesh 上最近点。out：`NearestResult` |
| `FindNearestRayIntersectionWithMesh` | `TargetMesh`, `RayOrigin`, `RayDirection`, `Options` | 射线与 mesh 求交。out：`HitResult` |
| `IsPointInsideMesh` | `TargetMesh`, `QueryPoint`, `Options` | 点是否在 mesh 内部。out：`bIsInside`（需要 BVH） |

注意：`FGeometryScriptDynamicMeshBVH` 不可序列化，BVH 只能在同一调用链中使用（如 Python 脚本内）。对于跨调用的空间查询，建议用 Python 脚本批量处理。

## 采样函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ComputePointSampling` | `TargetMesh`, `Options` | 泊松盘采样 |
| `ComputeUniformRandomPointSampling` | `TargetMesh`, `Options` | 均匀随机采样 |
| `ComputeNonUniformPointSampling` | `TargetMesh`, `Options` | 非均匀采样 |
| `ComputeVertexWeightedPointSampling` | `TargetMesh`, `Options` | 顶点权重采样 |

## 比较函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `IsSameMeshAs` | `TargetMesh`, `OtherMesh`, `Options` | 两个 mesh 是否相同 |
| `MeasureDistancesBetweenMeshes` | `TargetMesh`, `OtherMesh` | 测量两个 mesh 间的距离 |
| `IsIntersectingMesh` | `TargetMesh`, `OtherMesh`, `TargetTransform`, `OtherTransform` | 两个 mesh 是否相交 |

## JSON 示例

**获取包围盒**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshQueryFunctions","function":"GetMeshBoundingBox","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0"}}
```

**获取三角形顶点位置**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshQueryFunctions","function":"GetTrianglePositions","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","TriangleID":0}}
```

**获取体积和表面积**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshQueryFunctions","function":"GetMeshVolumeArea","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0"}}
```

## 发现

对上述任意 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。
