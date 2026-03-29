---
name: unreal-modeling-meshops
description: GeometryScript 网格处理操作。当用户要求修复、简化、Remesh、布尔运算、体素化、分解、基础编辑或细分网格时使用。
---

# 建模 — 网格操作

前置条件：先阅读 `unreal-modeling`。

## 库

| 库 | CDO |
|----|-----|
| 修复 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshRepairFunctions` |
| 简化 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSimplifyFunctions` |
| Remesh | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_RemeshingFunctions` |
| 布尔 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshBooleanFunctions` |
| 体素 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshVoxelFunctions` |
| 分解 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshDecompositionFunctions` |
| 基础编辑 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshBasicEditFunctions` |
| 细分 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSubdivideFunctions` |
| OpenSubdiv | `/Script/GeometryScriptingEditor.Default__GeometryScriptLibrary_OpenSubdivFunctions` |

## 修复函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CompactMesh` | `TargetMesh` | 压缩顶点/三角形 ID 空间，移除间隙 |
| `RemoveUnusedVertices` | `TargetMesh` | 删除未被三角形引用的顶点 |
| `WeldMeshEdges` | `TargetMesh`, `WeldOptions` | 焊接开放边界（修复裂缝） |
| `ResolveMeshTJunctions` | `TargetMesh`, `ResolveOptions` | 解决 T 形交叉 |
| `SnapMeshOpenBoundaries` | `TargetMesh`, `SnapOptions` | 吸附开放边界到最近的兼容边界 |
| `FillAllMeshHoles` | `TargetMesh`, `FillOptions` | 填充所有孔洞。out：`NumFilledHoles`、`NumFailedHoleFills` |
| `RemoveSmallComponents` | `TargetMesh`, `Options` | 删除体积/面积/三角形数低于阈值的孤立组件 |
| `RemoveHiddenTriangles` | `TargetMesh`, `Options` | 删除从外部不可见的三角形 |
| `SplitMeshBowties` | `TargetMesh`, `bMeshBowties`, `bAttributeBowties` | 拆分蝴蝶结拓扑 |
| `RepairMeshDegenerateGeometry` | `TargetMesh`, `Options` | 删除退化三角形（面积/边长过小） |

### 修复结构体

**`FGeometryScriptWeldEdgesOptions`**：`Tolerance`(float, 1e-6)、`bOnlyUniquePairs`(bool, true)

**`FGeometryScriptFillHolesOptions`**：`FillMethod`(枚举)、`bDeleteIsolatedTriangles`(bool, true)

**`EGeometryScriptFillHolesMethod`**：`Automatic`、`MinimalFill`、`PolygonTriangulation`、`TriangleFan`、`PlanarProjection`

**`FGeometryScriptRemoveSmallComponentOptions`**：`MinVolume`(0.0001)、`MinArea`(0.0001)、`MinTriangleCount`(1)

**`FGeometryScriptDegenerateTriangleOptions`**：`Mode`(枚举)、`MinTriangleArea`(0.001)、`MinEdgeLength`(0.0001)、`bCompactOnCompletion`(true)

**`EGeometryScriptRepairMeshMode`**：`DeleteOnly`、`RepairOrDelete`、`RepairOrSkip`

## 简化函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplySimplifyToTriangleCount` | `TargetMesh`, `TriangleCount`, `Options` | 简化到目标三角形数 |
| `ApplySimplifyToVertexCount` | `TargetMesh`, `VertexCount`, `Options` | 简化到目标顶点数 |
| `ApplySimplifyToTolerance` | `TargetMesh`, `Tolerance`, `Options` | 简化到几何容差 |
| `ApplySimplifyToEdgeLength` | `TargetMesh`, `EdgeLength`, `Options` | 简化到目标边长 |
| `ApplySimplifyToPlanar` | `TargetMesh`, `Options` | 简化平面区域（不改变 3D 形状） |
| `ApplySimplifyToPolygroupTopology` | `TargetMesh`, `Options`, `GroupLayer` | 简化到 Polygroup 拓扑 |
| `ApplyClusterSimplifyToEdgeLength` | `TargetMesh`, `EdgeLength`, `Options` | 基于聚类的快速简化 |

### 简化结构体

**`FGeometryScriptSimplifyMeshOptions`**

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Method` | `EGeometryScriptRemoveMeshSimplificationType` | `AttributeAware` | 简化算法 |
| `bAllowSeamCollapse` | bool | `true` | 允许接缝折叠 |
| `bAllowSeamSmoothing` | bool | `true` | 允许接缝平滑 |
| `bAllowSeamSplits` | bool | `true` | 允许接缝分裂 |
| `bPreserveVertexPositions` | bool | `false` | 保留顶点位置 |
| `bAutoCompact` | bool | `true` | 自动压缩 ID 空间 |

**`EGeometryScriptRemoveMeshSimplificationType`**：`StandardQEM`、`VolumePreserving`、`AttributeAware`

## Remesh 函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplyUniformRemesh` | `TargetMesh`, `RemeshOptions`, `UniformOptions` | 均匀重新网格化 |

**`FGeometryScriptRemeshOptions`**：`bDiscardAttributes`(false)、`bReprojectToInput`(true)

**`FGeometryScriptUniformRemeshOptions`**：`TargetType`(枚举)、`TargetEdgeLength`(3.0)、`TargetTriangleCount`(1000)、`SmoothingRate`(0.25)、`Iterations`(20)

## 布尔函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplyMeshBoolean` | `TargetMesh`, `TargetTransform`, `ToolMesh`, `ToolTransform`, `BooleanOperation`, `Options` | 布尔运算（并/差/交） |
| `ApplyMeshSelfUnion` | `TargetMesh`, `Options` | 自合并（消除自相交） |
| `ApplyMeshPlaneCut` | `TargetMesh`, `CutFrame`, `Options` | 平面切割 |
| `ApplyMeshPlaneSlice` | `TargetMesh`, `CutFrame`, `Options` | 平面切片（保留两侧） |
| `ApplyMeshMirror` | `TargetMesh`, `MirrorFrame`, `Options` | 镜像 |

**布尔操作枚举**（`BooleanOperation` 参数）：`Union`、`Subtract`、`Intersect`

## 体素函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplyMeshSolidify` | `TargetMesh`, `Options` | 体素化实体化 |
| `ApplyMeshMorphology` | `TargetMesh`, `Options` | 体素形态学操作（膨胀/腐蚀/开/闭） |

**`EGeometryScriptMorphologicalOpType`**：`Dilate`、`Contract`、`Close`、`Open`

## 基础编辑函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `AppendMesh` | `TargetMesh`, `AppendMesh` | 追加另一个 mesh |
| `AppendMeshTransformed` | `TargetMesh`, `AppendMesh`, `AppendTransform` | 带变换追加 |
| `AppendMeshRepeated` | `TargetMesh`, `AppendMesh`, `AppendTransform`, `RepeatCount` | 重复追加 |
| `SetVertexPosition` | `TargetMesh`, `VertexID`, `NewPosition`, `bDeferChangeNotifications` | 设置单个顶点位置 |
| `AddVertexToMesh` | `TargetMesh`, `NewPosition` | 添加顶点。out：`NewVertexIndex` |
| `AddVerticesToMesh` | `TargetMesh`, `NewPositionList` | 批量添加顶点。out：`NewIndicesList` |
| `AddTriangleToMesh` | `TargetMesh`, `NewTriangle` | 添加三角形。out：`NewTriangleIndex` |
| `AddTrianglesToMesh` | `TargetMesh`, `NewTrianglesList` | 批量添加三角形 |
| `DeleteTrianglesFromMesh` | `TargetMesh`, `TriangleList` | 删除三角形 |
| `DeleteSelectedTrianglesFromMesh` | `TargetMesh`, `Selection` | 删除选中三角形 |
| `DiscardMeshAttributes` | `TargetMesh` | 丢弃所有属性（法线/UV/颜色等） |
| `SetAllMeshVertexPositions` | `TargetMesh`, `PositionList` | 批量设置所有顶点位置 |

## 细分函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplySelectiveTessellation` | `TargetMesh`, `Selection`, `Options` | 选择性细分 |
| `ApplyUniformTessellation` | `TargetMesh`, `TessellationLevel` | 均匀细分 |
| `ApplyPNTessellation` | `TargetMesh`, `Options`, `TessellationLevel` | PN 三角形细分 |
| `ApplyPolygroupCatmullClarkSubD` | `TargetMesh`, `Subdivisions`, `GroupLayer` | Catmull-Clark 细分（Editor 库） |
| `ApplyTriangleLoopSubD` | `TargetMesh`, `Subdivisions` | Loop 细分（Editor 库） |

## JSON 示例

**填充孔洞**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshRepairFunctions","function":"FillAllMeshHoles","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","FillOptions":{"FillMethod":"Automatic","bDeleteIsolatedTriangles":true}}}
```

**简化到目标三角形数**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSimplifyFunctions","function":"ApplySimplifyToTriangleCount","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","TriangleCount":5000,"Options":{"Method":"AttributeAware","bAllowSeamCollapse":true,"bAutoCompact":true}}}
```

**布尔差集**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshBooleanFunctions","function":"ApplyMeshBoolean","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","TargetTransform":{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":0,"Y":0,"Z":0},"Scale3D":{"X":1,"Y":1,"Z":1}},"ToolMesh":"/Engine/Transient.DynamicMesh_1","ToolTransform":{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":0,"Y":0,"Z":0},"Scale3D":{"X":1,"Y":1,"Z":1}},"BooleanOperation":"Subtract","Options":{}}}
```

**均匀 Remesh**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_RemeshingFunctions","function":"ApplyUniformRemesh","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","RemeshOptions":{"bDiscardAttributes":false,"bReprojectToInput":true},"UniformOptions":{"TargetEdgeLength":3.0,"SmoothingRate":0.25,"Iterations":20}}}
```

## 发现

对上述任意 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。
