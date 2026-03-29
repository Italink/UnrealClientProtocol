---
name: unreal-modeling-attributes
description: GeometryScript 网格属性操作。当用户要求设置/查询顶点色、Polygroup、材质 ID 时使用。
---

# 建模 — 属性

前置条件：先阅读 `unreal-modeling`。

## 库

| 库 | CDO |
|----|-----|
| 顶点色 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshVertexColorFunctions` |
| Polygroup | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPolygroupFunctions` |
| 材质 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshMaterialFunctions` |

`TargetMesh` 为 `UDynamicMesh*` 路径字符串。`Debug` 省略。需要 `FGeometryScriptMeshSelection` 的函数，用 `MeshOperationLibrary` 桥接。需要 `FGeometryScriptColorList` 的函数，用 `MakeColorList`/`GetColorListArray` 桥接。

## 顶点色函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetMeshConstantVertexColor` | `TargetMesh`, `Color`(FLinearColor), `Flags`, `bClearExisting` | 设置全 mesh 统一顶点色 |
| `SetMeshSelectionVertexColor` | `TargetMesh`, `Selection`, `Color`, `Flags`, `bCreateColorSeam` | 设置选中区域顶点色 |
| `SetMeshPerVertexColors` | `TargetMesh`, `VertexColorList` | 批量设置每顶点颜色（列表大小 = MaxVertexID） |
| `GetMeshPerVertexColors` | `TargetMesh` | out：`VertexColorList`、`bIsValidColorSet`、`bHasVertexIDGaps`、`bHasSplitColors` |
| `SetMeshTriangleVertexColor` | `TargetMesh`, `TriangleID`, `Color`, `Flags`, `bDeferChangeNotifications` | 设置单个三角形顶点色 |
| `GetMeshTriangleVertexColors` | `TargetMesh`, `TriangleID` | 返回三角形三个角的顶点色 |
| `BlurMeshVertexColors` | `TargetMesh`, `Selection`, `Options` | 模糊/平滑顶点色 |
| `TransferVertexColorFromMesh` | `TargetMesh`, `SourceMesh`, `Options` | 从另一个 mesh 传输顶点色 |

### `FGeometryScriptColorFlags`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bRed` | bool | `true` | 影响 R 通道 |
| `bGreen` | bool | `true` | 影响 G 通道 |
| `bBlue` | bool | `true` | 影响 B 通道 |
| `bAlpha` | bool | `true` | 影响 A 通道 |

## Polygroup 函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `SetNumExtendedPolygroupLayers` | `TargetMesh`, `NumLayers` | 设置扩展 Polygroup 层数 |
| `ClearPolygroups` | `TargetMesh`, `GroupLayer` | 清除 Polygroup（全部设为 0） |
| `SetPolygroupForMeshSelection` | `TargetMesh`, `GroupLayer`, `Selection`, `SetPolygroupIDOut` | 设置选中区域的 Polygroup ID |
| `SetGroupIDForMeshSelection` | `TargetMesh`, `GroupLayer`, `Selection`, `NewGroupID` | 设置选中区域为指定 Group ID |
| `GetTrianglePolygroupID` | `TargetMesh`, `GroupLayer`, `TriangleID` | 获取三角形的 Polygroup ID |
| `GetAllPolygroupIDs` | `TargetMesh`, `GroupLayer` | 获取所有 Polygroup ID 列表 |
| `GetPolygroupIDsInMeshSelection` | `TargetMesh`, `GroupLayer`, `Selection` | 获取选中区域内的 Polygroup ID 列表 |
| `ComputePolygroupsFromAngleThreshold` | `TargetMesh`, `GroupLayer`, `CreaseAngle`, `MinGroupSize` | 按角度阈值自动计算 Polygroup |
| `ComputePolygroupsFromPolygonDetection` | `TargetMesh`, `GroupLayer`, `bRespectUVSeams`, `bRespectHardNormals`, `QuadAdjacencyWeight`, `MinQuadCount` | 按多边形检测计算 Polygroup |
| `CopyPolygroupsLayer` | `TargetMesh`, `FromGroupLayer`, `ToGroupLayer` | 复制 Polygroup 层 |
| `ConvertComponentsToPolygroups` | `TargetMesh`, `GroupLayer` | 将连通分量转为 Polygroup |
| `ConvertUVIslandsToPolygroups` | `TargetMesh`, `GroupLayer`, `UVLayer` | 将 UV 岛转为 Polygroup |
| `DeleteTrianglesInPolygroup` | `TargetMesh`, `GroupLayer`, `PolygroupID` | 删除指定 Polygroup 的三角形 |

### `FGeometryScriptGroupLayer`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bDefaultLayer` | bool | `true` | 使用默认层 |
| `ExtendedLayerIndex` | int | `0` | 扩展层索引（当 bDefaultLayer=false 时） |

## 材质函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `EnableMaterialIDs` | `TargetMesh` | 启用材质 ID 属性 |
| `SetMaterialIDOnTriangles` | `TargetMesh`, `TriangleIDList`, `MaterialID` | 设置指定三角形的材质 ID |
| `SetMaterialIDForMeshSelection` | `TargetMesh`, `Selection`, `MaterialID` | 设置选中区域的材质 ID |
| `SetAllMeshMaterialIDs` | `TargetMesh`, `TriangleMaterialIDList` | 批量设置所有三角形的材质 ID |
| `GetTriangleMaterialID` | `TargetMesh`, `TriangleID` | 获取三角形的材质 ID |
| `GetAllTriangleMaterialIDs` | `TargetMesh` | 获取所有三角形的材质 ID 列表 |
| `GetMaterialIDsOfMesh` | `TargetMesh` | 获取 mesh 中使用的所有材质 ID 集合 |
| `RemapMaterialIDs` | `TargetMesh`, `FromMaterialID`, `ToMaterialID` | 重映射材质 ID |
| `SetPolygroupMaterialID` | `TargetMesh`, `GroupLayer`, `PolygroupID`, `MaterialID` | 按 Polygroup 设置材质 ID |
| `DeleteTrianglesByMaterialID` | `TargetMesh`, `MaterialID` | 删除指定材质 ID 的三角形 |
| `GetMaxMaterialID` | `TargetMesh` | 获取最大材质 ID |
| `CompactMaterialIDs` | `TargetMesh` | 压缩材质 ID（移除未使用的间隙） |

## JSON 示例

**设置全 mesh 统一顶点色**（红色）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshVertexColorFunctions","function":"SetMeshConstantVertexColor","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Color":{"R":1,"G":0,"B":0,"A":1},"Flags":{"bRed":true,"bGreen":true,"bBlue":true,"bAlpha":true},"bClearExisting":true}}
```

**按角度阈值计算 Polygroup**（30° 折痕）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPolygroupFunctions","function":"ComputePolygroupsFromAngleThreshold","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","GroupLayer":{"bDefaultLayer":true,"ExtendedLayerIndex":0},"CreaseAngle":30,"MinGroupSize":2}}
```

**设置材质 ID**（将选中区域设为材质 1）：

先用 `MeshOperationLibrary.SelectTrianglesByIDs` 创建选择，然后：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshMaterialFunctions","function":"SetMaterialIDForMeshSelection","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Selection":"<从桥接函数获取>","MaterialID":1}}
```

## 发现

对上述任意 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。
