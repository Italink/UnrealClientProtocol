---
name: unreal-modeling-normals
description: GeometryScript 法线与切线操作。当用户要求重算法线、分裂法线、翻转法线、计算切线或修复法线方向时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-modeling
  tags: "geometryscript normals tangents flip split"
  updated: "2026-03-29"
---

# 建模 — 法线与切线

## 库

CDO：`/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshNormalsFunctions`

`TargetMesh` 为 `UDynamicMesh*` 路径字符串。`Debug` 省略。需要 `FGeometryScriptMeshSelection` 的函数，用 `MeshOperationLibrary` 桥接。需要 `FGeometryScriptVectorList` 的函数，用 `MakeVectorList`/`GetVectorListArray` 桥接。

## 主要函数

### 法线计算

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetPerVertexNormals` | `TargetMesh` | 平滑法线（无硬边） |
| `SetPerFaceNormals` | `TargetMesh` | 面法线（每条边都是硬边） |
| `RecomputeNormals` | `TargetMesh`, `CalculateOptions`, `bDeferChangeNotifications` | 保留已有分裂，重新计算法线 |
| `RecomputeNormalsForMeshSelection` | `TargetMesh`, `Selection`, `CalculateOptions` | 对选中区域重算法线 |
| `ComputeSplitNormals` | `TargetMesh`, `SplitOptions`, `CalculateOptions` | 按角度/Polygroup 重建分裂后重算法线 |
| `AutoRepairNormals` | `TargetMesh` | 修复不一致的法线方向（闭合 mesh 效果最佳） |

### 法线翻转

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `FlipNormals` | `TargetMesh` | 翻转全部法线并反转三角形绕序 |
| `FlipTriangleSelectionNormals` | `TargetMesh`, `Selection`, `bFlipTriangleOrientation`, `bFlipNormalDirection` | 翻转选中区域法线 |

### 法线读写

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetMeshTriangleNormals` | `TargetMesh`, `TriangleID`, `Normals`(FGeometryScriptTriangle) | 设置单个三角形三个角的法线。out：`bIsValidTriangle` |
| `SetMeshPerVertexNormals` | `TargetMesh`, `VertexNormalList` | 批量设置每顶点法线（列表大小 = MaxVertexID） |
| `GetMeshPerVertexNormals` | `TargetMesh`, `bAverageSplitVertexValues` | out：`NormalList`、`bIsValidNormalSet`、`bHasVertexIDGaps` |
| `UpdateVertexNormal` | `TargetMesh`, `VertexID`, `bUpdateNormal`, `NewNormal`, `bUpdateTangents`, `NewTangentX`, `NewTangentY` | 更新单个顶点的法线和/或切线 |
| `SetSplitNormalsAlongSelectedEdges` | `TargetMesh`, `Selection`, `bSplit`, `bRecalculateNormals`, `CalculateOptions` | 沿选中边分裂/合并法线 |

### 切线

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ComputeTangents` | `TargetMesh`, `Options` | 计算切线（需要有效法线 + UV） |
| `SetMeshPerVertexTangents` | `TargetMesh`, `TangentXList`, `TangentYList` | 批量设置切线 |
| `GetMeshPerVertexTangents` | `TargetMesh`, `bAverageSplitVertexValues` | out：`TangentXList`、`TangentYList` |
| `GetMeshHasTangents` | `TargetMesh` | out：`bHasTangents` |
| `DiscardTangents` | `TargetMesh` | 移除切线层 |

## 结构体与枚举

### `FGeometryScriptCalculateNormalsOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bAngleWeighted` | bool | `true` | 按角度加权 |
| `bAreaWeighted` | bool | `true` | 按面积加权 |

### `FGeometryScriptSplitNormalsOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bSplitByOpeningAngle` | bool | `true` | 按折痕角度分裂 |
| `OpeningAngleDeg` | float | `15` | 折痕角度阈值（度） |
| `bSplitByFaceGroup` | bool | `false` | 按 Polygroup 边界分裂 |
| `GroupLayer` | `FGeometryScriptGroupLayer` | 默认层 | Polygroup 层 |

### `FGeometryScriptTangentsOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Type` | `EGeometryScriptTangentTypes` | `FastMikkT` | 切线算法 |
| `UVLayer` | int | `0` | 用于计算切线的 UV 通道 |

### `EGeometryScriptTangentTypes`

| 值 | 说明 |
|----|------|
| `FastMikkT` | 快速 MikkT 切线 |
| `PerTriangle` | 每三角形切线 |
| `StandardMikkT` | 标准 MikkT（可能回退到 FastMikkT） |

## JSON 示例

**重算法线**（保留分裂，面积+角度加权）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshNormalsFunctions","function":"RecomputeNormals","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","CalculateOptions":{"bAreaWeighted":true,"bAngleWeighted":true},"bDeferChangeNotifications":false}}
```

**按角度分裂法线后重算**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshNormalsFunctions","function":"ComputeSplitNormals","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","SplitOptions":{"bSplitByOpeningAngle":true,"OpeningAngleDeg":60,"bSplitByFaceGroup":false},"CalculateOptions":{"bAreaWeighted":true,"bAngleWeighted":true}}}
```

**计算切线**（MikkT，UV 通道 0）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshNormalsFunctions","function":"ComputeTangents","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Options":{"Type":"FastMikkT","UVLayer":0}}}
```

## 发现

对 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)