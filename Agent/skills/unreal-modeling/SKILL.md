---
name: unreal-modeling
description: 通过 GeometryScript + UCP 进行程序化网格建模。当用户要求创建、修改、修复、简化、UV、布尔、变形或以其他方式处理网格时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-client-protocol
  tags: "geometryscript mesh modeling procedural"
  updated: "2026-03-29"
---

# 网格建模

建模通过两条路径完成：**MeshOperationLibrary**（生命周期管理 + 类型桥接）和 **GeometryScript 原生库**（600+ 函数，通过 UCP 通用 `call` 直接调用）。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。传输层规则见 `unreal-client-protocol`。

## MeshOperationLibrary

CDO：`/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary`

### 生命周期

| 函数 | 参数 | 说明 |
|------|------|------|
| `CreateDynamicMesh` | — | 创建空的临时 DynamicMesh，返回对象路径 |
| `ReleaseDynamicMesh` | `Mesh` | 标记 mesh 等待 GC 回收 |
| `ReleaseAllDynamicMeshes` | — | 释放所有已跟踪的 mesh |
| `ListDynamicMeshes` | — | 列出所有已跟踪 mesh 的路径 |

### 摘要

| 函数 | 参数 | 说明 |
|------|------|------|
| `GetMeshSummary` | `TargetMesh` | 返回 JSON：vertexCount、triangleCount、uvSetCount、bounds、hasNormals、hasVertexColors、hasMaterialIDs、hasPolygroups、isClosed、isDense |

### 资产读写

| 函数 | 参数 | 说明 |
|------|------|------|
| `CopyFromStaticMesh` | `TargetMesh`, `SourceStaticMesh` | 将 StaticMesh 加载到 DynamicMesh（默认选项） |
| `CopyToStaticMesh` | `SourceMesh`, `TargetStaticMesh` | 将 DynamicMesh 写回已有 StaticMesh |
| `CopyToNewStaticMeshAsset` | `SourceMesh`, `AssetPath` | 创建新 StaticMesh 资产，返回资产路径 |

### Selection 桥接

GeometryScript 的 `FGeometryScriptMeshSelection` 内部使用不可序列化的 `TSharedPtr` 存储数据，无法通过 JSON 传递。以下函数从可序列化的输入创建 Selection：

| 函数 | 参数 | 说明 |
|------|------|------|
| `SelectAllTriangles` | `TargetMesh` | 选择全部三角形 |
| `SelectTrianglesByIDs` | `TargetMesh`, `TriangleIDs`(int 数组) | 按 ID 选择三角形 |
| `SelectVerticesByIDs` | `TargetMesh`, `VertexIDs`(int 数组) | 按 ID 选择顶点 |
| `SelectPolygroupByID` | `TargetMesh`, `PolygroupID`, `GroupLayer` | 按 Polygroup 选择 |
| `SelectMaterialByID` | `TargetMesh`, `MaterialID` | 按材质 ID 选择 |
| `GetSelectionIDs` | `Selection` | 从 Selection 提取 ID 数组 |

### 数据类型桥接

GeometryScript 的列表类型（`FGeometryScriptIndexList`、`FGeometryScriptVectorList` 等）将数据存储在不可序列化的 `TSharedPtr<TArray<...>>` 中。用 `Make*` 从数组创建，用 `Get*Array` 提取：

| Make 函数 | Get 函数 | 桥接类型 |
|-----------|----------|----------|
| `MakeIndexList(Indices, IndexType)` | `GetIndexListArray` | `FGeometryScriptIndexList` |
| `MakeVectorList(Vectors)` | `GetVectorListArray` | `FGeometryScriptVectorList` |
| `MakePolyPath(Points, bClosedLoop)` | `GetPolyPathArray` | `FGeometryScriptPolyPath` |
| `MakeSimplePolygon(Vertices)` | `GetSimplePolygonArray` | `FGeometryScriptSimplePolygon` |
| `MakeColorList(Colors)` | `GetColorListArray` | `FGeometryScriptColorList` |
| `MakeUVList(UVs)` | `GetUVListArray` | `FGeometryScriptUVList` |
| `MakeScalarList(Scalars)` | `GetScalarListArray` | `FGeometryScriptScalarList` |
| `MakeTriangleList(Triangles)` | `GetTriangleListArray` | `FGeometryScriptTriangleList` |

## 直接调用 GeometryScript

GeometryScript 函数是标准的 `BlueprintCallable` 静态函数，通过 UCP 通用 `call` + CDO 路径调用。

### CDO 路径格式

```
/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_<类名>
```

示例：`/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshRepairFunctions`

### 参数规则

- **`UDynamicMesh*`**：传路径字符串，如 `"/Engine/Transient.DynamicMesh_0"`
- **`USTRUCT` Options**：传 JSON 对象，只写需要覆盖的字段，省略的字段使用默认值
- **`UENUM`**：传字符串，即枚举值名，如 `"Automatic"`、`"AttributeAware"`
- **`out` 参数**（如 `Outcome`、`NumFilledHoles`）：输入时省略，结果中自动返回
- **`Debug`**：省略（默认 nullptr）
- **`WorldContextObject`**：UCP 自动注入

### 返回值

多数 GeometryScript 函数返回 `UDynamicMesh*`（同一个对象，原地修改）。响应中包含 mesh 路径和所有 out 参数。

## 标准工作流

### 示例 1：加载 → 简化 → 写回

```json
// 1. 创建工作 mesh
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"CreateDynamicMesh"}

// 2. 从 StaticMesh 加载
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"CopyFromStaticMesh","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","SourceStaticMesh":"/Game/Meshes/SM_Rock.SM_Rock"}}

// 3. 简化到 5000 三角形
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSimplifyFunctions","function":"ApplySimplifyToTriangleCount","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","TriangleCount":5000,"Options":{"Method":"AttributeAware"}}}

// 4. 写回
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"CopyToStaticMesh","params":{"SourceMesh":"/Engine/Transient.DynamicMesh_0","TargetStaticMesh":"/Game/Meshes/SM_Rock.SM_Rock"}}

// 5. 释放
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"ReleaseDynamicMesh","params":{"Mesh":"/Engine/Transient.DynamicMesh_0"}}
```

### 示例 2：创建图元 → 保存为新资产

```json
// 1. 创建 mesh
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"CreateDynamicMesh"}

// 2. 追加 Box 图元
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions","function":"AppendBox","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","PrimitiveOptions":{},"DimensionX":100,"DimensionY":100,"DimensionZ":100}}

// 3. 自动生成 UV
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshUVFunctions","function":"AutoGenerateXAtlasMeshUVs","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","UVSetIndex":0,"Options":{}}}

// 4. 保存为新资产
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"CopyToNewStaticMeshAsset","params":{"SourceMesh":"/Engine/Transient.DynamicMesh_0","AssetPath":"/Game/Generated/SM_Box"}}

// 5. 释放
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"ReleaseDynamicMesh","params":{"Mesh":"/Engine/Transient.DynamicMesh_0"}}
```

### 示例 3：布尔合并

```json
// 1-2. 创建两个 mesh 并填充（CreateDynamicMesh + AppendSphere / AppendBox）

// 3. 布尔合并
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshBooleanFunctions","function":"ApplyMeshBoolean","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","TargetTransform":{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":0,"Y":0,"Z":0},"Scale3D":{"X":1,"Y":1,"Z":1}},"ToolMesh":"/Engine/Transient.DynamicMesh_1","ToolTransform":{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":50,"Y":0,"Z":0},"Scale3D":{"X":1,"Y":1,"Z":1}},"BooleanOperation":"Union","Options":{}}}

// 4. 释放 mesh B
{"object":"/Script/UnrealClientProtocolEditor.Default__MeshOperationLibrary","function":"ReleaseDynamicMesh","params":{"Mesh":"/Engine/Transient.DynamicMesh_1"}}
```

## GeometryScript 库索引

每个库对应一个域 Skill，包含完整的函数签名、Options 结构体字段、枚举值和 JSON 示例：

| 域 Skill | 覆盖的库 |
|----------|----------|
| `unreal-modeling-primitives` | MeshPrimitiveFunctions、MeshModelingFunctions、PolyPathFunctions、SimplePolygonFunctions、PolygonListFunctions |
| `unreal-modeling-meshops` | MeshRepairFunctions、MeshSimplifyFunctions、RemeshingFunctions、MeshBooleanFunctions、MeshVoxelFunctions、MeshDecompositionFunctions、MeshBasicEditFunctions、MeshSubdivideFunctions、OpenSubdivFunctions |
| `unreal-modeling-uv` | MeshUVFunctions |
| `unreal-modeling-normals` | MeshNormalsFunctions |
| `unreal-modeling-deform` | MeshDeformFunctions、MeshTransformFunctions |
| `unreal-modeling-query` | MeshQueryFunctions、MeshSpatialFunctions、MeshSamplingFunctions、ContainmentFunctions、MeshComparisonFunctions、MeshGeodesicFunctions |
| `unreal-modeling-selection` | MeshSelectionFunctions |
| `unreal-modeling-attributes` | MeshVertexColorFunctions、MeshPolygroupFunctions、MeshMaterialFunctions |
| `unreal-modeling-asset` | StaticMeshFunctions、CreateNewAssetFunctions、CollisionFunctions、EditorDynamicMeshFunctions |

## 关键规则

1. **先创建再操作**：调用 GeometryScript 前必须有 DynamicMesh（通过 `CreateDynamicMesh` 或 `CopyFromStaticMesh` 获取）
2. **用完即释放**：调用 `ReleaseDynamicMesh` 避免临时对象堆积
3. **原地修改**：多数 GeometryScript 函数原地修改 mesh 并返回同一个 `UDynamicMesh*`
4. **Options 可选**：USTRUCT Options 参数有合理默认值，只传需要覆盖的字段
5. **不可序列化类型用桥接函数**：当 GeometryScript 函数需要 `FGeometryScriptMeshSelection`、`FGeometryScriptIndexList`、`FGeometryScriptPolyPath` 等类型时，用 MeshOperationLibrary 的桥接函数从普通数组创建
6. **用 `DescribeObject` 探索**：对任意 GeometryScript CDO 调用 `DescribeObject` 可查看完整函数列表和签名

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)