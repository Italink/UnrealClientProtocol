---
name: unreal-modeling-selection
description: GeometryScript 网格选择操作。当用户要求按条件选择网格元素（球形/盒体/平面/法线角度/材质等）、组合选择、扩展/收缩选择时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-modeling
  tags: "geometryscript selection sphere box plane material"
  updated: "2026-03-29"
---

# 建模 — 选择

## 重要说明

`FGeometryScriptMeshSelection` 内部使用不可序列化的 `TSharedPtr<FGeometrySelection>` 存储数据，**无法通过 UCP JSON 直接创建或传递选择内容**。

**创建选择的两种方式**：

1. **MeshOperationLibrary 桥接函数**（推荐用于简单选择）：`SelectAllTriangles`、`SelectTrianglesByIDs`、`SelectVerticesByIDs`、`SelectPolygroupByID`、`SelectMaterialByID`——见 `unreal-modeling` 核心 Skill
2. **GeometryScript 原生选择函数**（本 Skill）：`SelectMeshElementsInSphere`、`SelectMeshElementsByNormalAngle` 等——适用于基于几何条件的复杂选择

创建后的 Selection 可作为参数传给其他 GeometryScript 函数（UV、变形、简化等）。

## 库

CDO：`/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSelectionFunctions`

## 创建选择

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CreateSelectAllMeshSelection` | `TargetMesh` | 选择全部。out：`Selection` |
| `SelectMeshElementsInSphere` | `TargetMesh`, `SphereOrigin`, `SphereRadius`, `SelectionType`, `MinNumTrianglePoints` | 球形范围选择。out：`Selection` |
| `SelectMeshElementsInBox` | `TargetMesh`, `BoxTransform`, `BoxDimensions`, `SelectionType`, `MinNumTrianglePoints` | 盒体范围选择 |
| `SelectMeshElementsWithPlane` | `TargetMesh`, `PlaneOrigin`, `PlaneNormal`, `SelectionType`, `MinNumTrianglePoints` | 平面一侧选择 |
| `SelectMeshElementsByNormalAngle` | `TargetMesh`, `Normal`, `MaxAngleDeg`, `SelectionType`, `MinNumTrianglePoints` | 按法线角度选择 |
| `SelectMeshElementsByMaterialID` | `TargetMesh`, `MaterialID` | 按材质 ID 选择 |

### `EGeometryScriptMeshSelectionType`

| 值 | 说明 |
|----|------|
| `Vertices` | 选择顶点 |
| `Edges` | 选择边 |
| `Triangles` | 选择三角形 |
| `Polygroups` | 选择 Polygroup |

### `MinNumTrianglePoints` 参数

控制三角形被选中的条件：`1` = 任意顶点在范围内即选中，`2` = 至少两个顶点，`3` = 全部顶点都在范围内。

## 转换选择

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ConvertMeshSelection` | `TargetMesh`, `FromSelection`, `NewType` | 在顶点/三角形/Polygroup 间转换。out：`ToSelection` |
| `ConvertMeshSelectionToIndexList` | `TargetMesh`, `Selection` | 转换为索引列表。out：`IndexList`、`SelectionType` |
| `ConvertIndexListToMeshSelection` | `TargetMesh`, `IndexList`, `SelectionType` | 从索引列表创建选择。out：`Selection` |
| `ConvertIndexArrayToMeshSelection` | `TargetMesh`, `IndexArray`, `SelectionType` | 从 int 数组创建选择 |

## 组合选择

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CombineMeshSelections` | `SelectionA`, `SelectionB`, `CombineMode` | 组合两个选择。out：`ResultSelection` |

### `EGeometryScriptCombineSelectionMode`

| 值 | 说明 |
|----|------|
| `Add` | 并集 |
| `Subtract` | 差集（A - B） |
| `Intersection` | 交集 |

## 修改选择

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `InvertMeshSelection` | `TargetMesh`, `Selection` | 反转选择。out：`NewSelection` |
| `ExpandMeshSelectionToConnected` | `TargetMesh`, `Selection` | 扩展到连通区域。out：`NewSelection` |
| `ExpandContractMeshSelection` | `TargetMesh`, `Selection`, `Iterations`, `bContract`, `bOnlyExpandToFaceNeighbours` | 扩展/收缩选择 |

## 查询选择

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetMeshSelectionInfo` | `Selection` | out：`SelectionType`、`NumSelected` |

## JSON 示例

**球形范围选择三角形**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSelectionFunctions","function":"SelectMeshElementsInSphere","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","SphereOrigin":{"X":0,"Y":0,"Z":50},"SphereRadius":100,"SelectionType":"Triangles","MinNumTrianglePoints":1}}
```

**按法线角度选择（朝上的面）**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshSelectionFunctions","function":"SelectMeshElementsByNormalAngle","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Normal":{"X":0,"Y":0,"Z":1},"MaxAngleDeg":45,"SelectionType":"Triangles","MinNumTrianglePoints":3}}
```

**组合两个选择（并集）**：

使用 Python 脚本在单次调用中完成多步选择组合，避免 Selection 的不可序列化问题：

```python
# 通过 ExecutePythonScript 在引擎内执行
import unreal
# ... 在脚本内创建、组合选择并传给后续操作
```

或者分步调用：先创建选择 A，再创建选择 B，然后组合——每步的 Selection 作为 out 参数返回后可在下一步作为输入使用（UCP 会在同一连接内保持对象引用）。

## 发现

对 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)