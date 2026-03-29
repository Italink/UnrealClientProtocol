---
name: unreal-modeling-uv
description: GeometryScript UV 操作。当用户要求生成、投影、布局、变换 UV 或进行 UV 相关处理时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-modeling
  tags: "geometryscript uv projection layout transform"
  updated: "2026-03-29"
---

# 建模 — UV

## 库

CDO：`/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshUVFunctions`

`TargetMesh` 为 `UDynamicMesh*` 路径字符串。`UVSetIndex`（别名 `UVChannel`）为 int，通常 `0`。`Debug` 省略。需要 `FGeometryScriptMeshSelection` 的函数，用 `MeshOperationLibrary` 桥接（见 `unreal-modeling`）。

## 主要函数

### UV 通道管理

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetNumUVSets` | `TargetMesh`, `NumUVSets` | 设置 UV 通道数量 |
| `ClearUVChannel` | `TargetMesh`, `UVChannel` | 清空指定 UV 通道 |
| `CopyUVSet` | `TargetMesh`, `FromUVSet`, `ToUVSet` | 复制 UV 通道 |

### UV 自动生成

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `AutoGenerateXAtlasMeshUVs` | `TargetMesh`, `UVSetIndex`, `Options` | XAtlas 自动展开（推荐通用方案） |
| `AutoGeneratePatchBuilderMeshUVs` | `TargetMesh`, `UVSetIndex`, `Options` | PatchBuilder 自动展开（更多控制） |
| `RecomputeMeshUVs` | `TargetMesh`, `UVSetIndex`, `Options`, `Selection` | 基于已有岛或 Polygroup 重新计算 UV |

### UV 投影

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetMeshUVsFromPlanarProjection` | `TargetMesh`, `UVSetIndex`, `PlaneTransform`, `Selection` | 平面投影。PlaneTransform 的 Scale 定义映射到 1 UV 的世界尺寸 |
| `SetMeshUVsFromBoxProjection` | `TargetMesh`, `UVSetIndex`, `BoxTransform`, `Selection`, `MinIslandTriCount` | 盒体投影 |
| `SetMeshUVsFromCylinderProjection` | `TargetMesh`, `UVSetIndex`, `CylinderTransform`, `Selection`, `SplitAngle` | 圆柱投影 |

### UV 布局与打包

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `RepackMeshUVs` | `TargetMesh`, `UVSetIndex`, `RepackOptions` | 重新打包 UV 岛 |
| `LayoutMeshUVs` | `TargetMesh`, `UVSetIndex`, `LayoutOptions`, `Selection` | 布局 UV（支持多种模式） |
| `ApplyTexelDensityUVScaling` | `TargetMesh`, `UVSetIndex`, `Options`, `Selection` | 按纹素密度缩放 UV |

### UV 变换

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `TranslateMeshUVs` | `TargetMesh`, `UVSetIndex`, `Translation`, `Selection` | 平移 UV |
| `ScaleMeshUVs` | `TargetMesh`, `UVSetIndex`, `Scale`, `ScaleOrigin`, `Selection` | 缩放 UV |
| `RotateMeshUVs` | `TargetMesh`, `UVSetIndex`, `RotationAngle`, `RotationOrigin`, `Selection` | 旋转 UV（角度制） |

### UV 查询

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetMeshUVSizeInfo` | `TargetMesh`, `UVSetIndex`, `Selection` | 返回 3D 面积、UV 面积、3D 包围盒、UV 包围盒等 |
| `GetMeshPerVertexUVs` | `TargetMesh`, `UVSetIndex` | 获取每顶点 UV 列表 |

### UV 传输

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `TransferMeshUVsByProjection` | `TargetMesh`, `TargetUVChannel`, `TargetSelection`, `TargetTransform`, `SourceMesh`, `SourceMeshOptionalBVH`, `SourceUVChannel`, `SourceSelection`, `SourceTransform`, `Settings`, `ProjectionDirection` | 通过投影从源 mesh 传输 UV |
| `SetUVSeamsAlongSelectedEdges` | `TargetMesh`, `UVChannel`, `Selection`, `bInsertSeams` | 沿选中边设置/移除 UV 接缝 |

### UV 底层操作

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetMeshTriangleUVs` | `TargetMesh`, `UVSetIndex`, `TriangleID`, `UVs` | 设置单个三角形的 UV |
| `AddUVElementToMesh` | `TargetMesh`, `UVSetIndex`, `NewUVPosition` | 添加 UV 元素 |
| `SetMeshUVElementPosition` | `TargetMesh`, `UVSetIndex`, `ElementID`, `NewUVPosition` | 设置 UV 元素位置 |
| `CopyMeshUVLayerToMesh` | `CopyFromMesh`, `UVSetIndex`, `CopyToUVMesh` | 将 UV 通道导出为独立 mesh |
| `CopyMeshToMeshUVLayer` | `CopyFromUVMesh`, `ToUVSetIndex`, `CopyToMesh` | 将 mesh 写回 UV 通道 |

## 结构体与枚举

### `FGeometryScriptRecomputeUVsOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Method` | `EGeometryScriptUVFlattenMethod` | `SpectralConformal` | 展平算法 |
| `IslandSource` | `EGeometryScriptUVIslandSource` | `UVIslands` | 岛来源 |
| `ExpMapOptions` | struct | — | ExpMap 专用选项 |
| `SpectralConformalOptions` | struct | — | SpectralConformal 专用选项 |
| `GroupLayer` | `FGeometryScriptGroupLayer` | 默认层 | Polygroup 层（当 IslandSource=PolyGroups 时） |
| `bAutoAlignIslandsWithAxes` | bool | `true` | 自动对齐岛到坐标轴 |

### `EGeometryScriptUVFlattenMethod`

| 值 | 说明 |
|----|------|
| `ExpMap` | 指数映射 |
| `Conformal` | 保角映射 |
| `SpectralConformal` | 谱保角映射（推荐） |

### `EGeometryScriptUVIslandSource`

| 值 | 说明 |
|----|------|
| `PolyGroups` | 按 Polygroup 分岛 |
| `UVIslands` | 按已有 UV 岛分岛 |

### `FGeometryScriptXAtlasOptions`

| 字段 | 类型 | 默认值 |
|------|------|--------|
| `MaxIterations` | int | `2` |

### `FGeometryScriptPatchBuilderOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `InitialPatchCount` | int | `100` | 初始 patch 数量 |
| `MinPatchSize` | int | `2` | 最小 patch 大小 |
| `PatchCurvatureAlignmentWeight` | float | `1.0` | 曲率对齐权重 |
| `PatchMergingMetricThresh` | float | `1.5` | 合并度量阈值 |
| `PatchMergingAngleThresh` | float | `45.0` | 合并角度阈值 |
| `bRespectInputGroups` | bool | `false` | 尊重输入 Polygroup |
| `bAutoPack` | bool | `true` | 自动打包 |
| `PackingOptions` | `FGeometryScriptRepackUVsOptions` | — | 打包选项 |

### `FGeometryScriptRepackUVsOptions`

| 字段 | 类型 | 默认值 |
|------|------|--------|
| `TargetImageWidth` | int | `512` |
| `bOptimizeIslandRotation` | bool | `true` |

### `FGeometryScriptLayoutUVsOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `LayoutType` | `EGeometryScriptUVLayoutType` | `Repack` | 布局类型 |
| `TextureResolution` | int | `1024` | 纹理分辨率 |
| `Scale` | float | `1` | 打包后缩放 |
| `Translation` | FVector2D | `(0,0)` | 打包后平移 |
| `bPreserveScale` | bool | `false` | 保留岛缩放 |
| `bPreserveRotation` | bool | `false` | 保留岛旋转 |
| `bAllowFlips` | bool | `false` | 允许翻转 |

### `EGeometryScriptUVLayoutType`

| 值 | 说明 |
|----|------|
| `Transform` | 应用 Scale/Translation |
| `Stack` | 每个岛独立缩放到单位正方形 |
| `Repack` | 集体打包到单位正方形（无重叠） |
| `Normalize` | 归一化纹素密度 |

## JSON 示例

**XAtlas 自动展开**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshUVFunctions","function":"AutoGenerateXAtlasMeshUVs","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","UVSetIndex":0,"Options":{"MaxIterations":2}}}
```

**盒体投影**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshUVFunctions","function":"SetMeshUVsFromBoxProjection","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","UVSetIndex":0,"BoxTransform":{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":0,"Y":0,"Z":0},"Scale3D":{"X":100,"Y":100,"Z":100}},"Selection":{},"MinIslandTriCount":2}}
```

**重新计算 UV（SpectralConformal）**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshUVFunctions","function":"RecomputeMeshUVs","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","UVSetIndex":0,"Options":{"Method":"SpectralConformal","IslandSource":"UVIslands","bAutoAlignIslandsWithAxes":true},"Selection":{}}}
```

## 发现

对 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)