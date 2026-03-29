---
name: unreal-modeling-asset
description: GeometryScript 资产读写与碰撞操作。当用户要求精细控制 StaticMesh 资产加载/保存选项、创建新资产、生成碰撞体时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-modeling
  tags: "geometryscript asset load save collision"
  updated: "2026-03-29"
---

# 建模 — 资产与碰撞

## 简化版 vs 原生版

**简单场景**优先使用 `MeshOperationLibrary` 的 `CopyFromStaticMesh`/`CopyToStaticMesh`/`CopyToNewStaticMeshAsset`（默认选项，一步完成）。

**本 Skill** 中的原生函数用于需要精细控制的场景：指定 LOD、自定义 Options、碰撞生成等。

## 库

| 库 | CDO |
|----|-----|
| StaticMesh 读写 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_StaticMeshFunctions` |
| 新建资产 | `/Script/GeometryScriptingEditor.Default__GeometryScriptLibrary_CreateNewAssetFunctions` |
| 碰撞 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_CollisionFunctions` |
| 编辑器 DynamicMesh | `/Script/GeometryScriptingEditor.Default__GeometryScriptLibrary_EditorDynamicMeshFunctions` |

## StaticMesh 读写函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CopyMeshFromStaticMeshV2` | `FromStaticMeshAsset`, `ToDynamicMesh`, `AssetOptions`, `RequestedLOD`, `bUseSectionMaterials` | 从 StaticMesh 加载到 DynamicMesh。out：`Outcome` |
| `CopyMeshToStaticMesh` | `FromDynamicMesh`, `ToStaticMeshAsset`, `Options`, `TargetLOD`, `bUseSectionMaterials` | 写回 StaticMesh。out：`Outcome` |
| `GetSectionMaterialListFromStaticMesh` | `FromStaticMeshAsset`, `RequestedLOD` | 获取材质列表。out：`MaterialList`、`MaterialIndex`、`Outcome` |

### `FGeometryScriptCopyMeshFromAssetOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bApplyBuildSettings` | bool | `true` | 应用构建设置 |
| `bRequestTangents` | bool | `true` | 请求切线 |
| `bIgnoreRemoveDegenerates` | bool | `true` | 忽略退化三角形移除 |

### `FGeometryScriptMeshReadLOD`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `LODType` | `EGeometryScriptLODType` | `MaxAvailable` | LOD 类型 |
| `LODIndex` | int | `0` | LOD 索引 |

### `EGeometryScriptLODType`

| 值 | 说明 |
|----|------|
| `MaxAvailable` | 最高质量可用 LOD |
| `HiResSourceModel` | 高分辨率源模型（编辑器专用） |
| `SourceModel` | 源模型指定 LOD |
| `RenderData` | 渲染数据 LOD（运行时需要 bAllowCPUAccess） |

### `FGeometryScriptCopyMeshToAssetOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bEnableRecomputeNormals` | bool | `false` | 重算法线 |
| `bEnableRecomputeTangents` | bool | `false` | 重算切线 |
| `bEnableRemoveDegenerates` | bool | `false` | 移除退化三角形 |

### `FGeometryScriptMeshWriteLOD`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bWriteHiResSource` | bool | `false` | 写入高分辨率源 |
| `LODIndex` | int | `0` | 目标 LOD 索引 |

## 新建资产函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CreateNewStaticMeshAssetFromMesh` | `FromDynamicMesh`, `AssetPathAndName`, `Options` | 从 DynamicMesh 创建新 StaticMesh 资产。out：`Outcome`，返回 `UStaticMesh*` |
| `CreateNewStaticMeshAssetFromMeshLODs` | `FromDynamicMeshLODs`(数组), `AssetPathAndName`, `Options` | 从多个 DynamicMesh 创建多 LOD 资产 |
| `CreateUniqueNewAssetPathName` | `AssetFolderPath`, `BaseAssetName`, `UniqueAssetPathAndName` | 生成唯一资产路径 |

## 碰撞函数

### 从 Mesh 生成碰撞

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetStaticMeshCollisionFromMesh` | `FromDynamicMesh`, `ToStaticMeshAsset`, `CollisionOptions`, `StaticMeshCollisionOptions` | 从 DynamicMesh 设置 StaticMesh 碰撞 |
| `SetDynamicMeshCollisionFromMesh` | `FromDynamicMesh`, `ToDynamicMeshComponent`, `CollisionOptions` | 设置 DynamicMeshComponent 碰撞 |
| `ResetDynamicMeshCollision` | `Component`, `bEmitTransaction` | 重置碰撞 |

### 简单碰撞生成

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ComputeSimpleCollision` | `TargetMesh`, `Options`, `SimpleCollision` | 计算简单碰撞体。out：`SimpleCollision` |
| `SetStaticMeshCollisionFromSimpleCollision` | `StaticMeshAsset`, `SimpleCollision` | 将简单碰撞应用到 StaticMesh |
| `AppendSimpleCollisionShapes` | `TargetMesh`, `SimpleCollision`, `Options` | 将碰撞形状追加为 mesh 几何体 |

### `FGeometryScriptCollisionFromMeshOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bEmitTransaction` | bool | `true` | 发出事务（支持撤销） |
| `Method` | `EGeometryScriptCollisionGenerationMethod` | `ConvexDecomposition` | 碰撞生成方法 |
| `bAutoDetectSpheres` | bool | `false` | 自动检测球体 |
| `bAutoDetectBoxes` | bool | `false` | 自动检测盒体 |
| `bAutoDetectCapsules` | bool | `false` | 自动检测胶囊体 |
| `MinThickness` | float | `1.0` | 最小厚度 |
| `bSimplifyHulls` | bool | `true` | 简化凸包 |
| `HullTargetFaceCount` | int | `25` | 凸包目标面数 |
| `MaxConvexHulls` | int | `4` | 最大凸包数 |

### `EGeometryScriptCollisionGenerationMethod`

| 值 | 说明 |
|----|------|
| `AlignedBoxes` | 轴对齐盒体 |
| `OrientedBoxes` | 有向盒体 |
| `MinimalSpheres` | 最小球体 |
| `Capsules` | 胶囊体 |
| `ProjectedHulls` | 投影凸包 |
| `SweptHulls` | 扫掠凸包 |
| `MinVolumeHulls` | 最小体积凸包 |
| `ConvexDecomposition` | 凸分解（最通用） |

## JSON 示例

**从 StaticMesh 加载指定 LOD**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_StaticMeshFunctions","function":"CopyMeshFromStaticMeshV2","params":{"FromStaticMeshAsset":"/Game/Meshes/SM_Rock.SM_Rock","ToDynamicMesh":"/Engine/Transient.DynamicMesh_0","AssetOptions":{"bApplyBuildSettings":true,"bRequestTangents":true},"RequestedLOD":{"LODType":"SourceModel","LODIndex":0},"bUseSectionMaterials":true}}
```

**写回 StaticMesh 并重算法线/切线**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_StaticMeshFunctions","function":"CopyMeshToStaticMesh","params":{"FromDynamicMesh":"/Engine/Transient.DynamicMesh_0","ToStaticMeshAsset":"/Game/Meshes/SM_Rock.SM_Rock","Options":{"bEnableRecomputeNormals":true,"bEnableRecomputeTangents":true,"bEnableRemoveDegenerates":true},"TargetLOD":{"bWriteHiResSource":false,"LODIndex":0},"bUseSectionMaterials":true}}
```

**生成碰撞（凸分解）**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_CollisionFunctions","function":"SetStaticMeshCollisionFromMesh","params":{"FromDynamicMesh":"/Engine/Transient.DynamicMesh_0","ToStaticMeshAsset":"/Game/Meshes/SM_Rock.SM_Rock","CollisionOptions":{"Method":"ConvexDecomposition","MaxConvexHulls":4,"bSimplifyHulls":true,"HullTargetFaceCount":25},"StaticMeshCollisionOptions":{}}}
```

## 发现

对上述任意 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)