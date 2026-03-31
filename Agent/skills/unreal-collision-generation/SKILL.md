---
name: unreal-collision-generation
description: >-
  Agent 驱动的碰撞体生成管线。当用户要求为 StaticMesh 生成高品质复杂碰撞体时使用。
metadata:
  version: "1.0.0"
  layer: domain
  tags: "collision mesh generation approximate solidify"
  updated: "2026-03-29"
---

# 碰撞体生成管线

通过 `UPneumaViewCollision` 为 StaticMesh 生成高品质复杂碰撞体。UE 侧内聚所有确定性逻辑（环境构建、布局、材质、统计、碰撞设置），Agent 只做需要判断力的决策（参数选择、效果评估）。

## 工作流

```
1. Open(SourceMesh)     -> 环境就绪 + 分析数据（含 suggestedParams）
2. 看分析数据，决定参数  -> Agent 判断
3. Generate(Params)      -> 碰撞体生成（异步） + 场景自动更新
4. Inspect()             -> 多视角截图 + 统计数据
5. 看截图+数据，判断效果  -> Agent 判断
6. Accept() 或回到步骤 2  -> Agent 判断
```

## UFunction 参考

Agent 只需要 5 个函数。`Open` 是静态函数（CDO 调用），其余在返回的实例上调用。

### Open（静态）

打开碰撞体编辑环境。自动完成：复制临时资产、打开编辑器、左右摆放模型、设置 Overlay 统计面板、相机对焦、分析网格。

```json
{
  "object": "/Script/UnrealClientProtocolEditor.Default__PneumaViewCollision",
  "function": "Open",
  "params": {
    "SourceMesh": "/Game/Meshes/SM_MyModel.SM_MyModel"
  }
}
```

返回 `UPneumaViewCollision` 实例路径（后续调用的 `object`）。返回的 JSON 中 `ReturnValue` 是实例路径。

### Generate（实例，DeferredResponse）

异步生成碰撞体。完成后自动替换预览场景中的碰撞体、更新统计面板、设为源网格的 ComplexCollisionMesh。

**重要**：这是异步操作，需要设置较长的超时：`$env:UE_TIMEOUT = "120"`

```json
{
  "object": "<Open返回的实例路径>",
  "function": "Generate",
  "params": {
    "Params": {
      "bWeldBoundaryEdges": true,
      "WeldTolerance": 0.001,
      "bFillSmallHoles": false,
      "MaxHoleArea": 0.5,
      "bThickenThinParts": false,
      "bOverrideShellThickness": false,
      "ShellThickness": 0.05,
      "VoxelSizeMode": "Resolution",
      "VoxelSize": 0.5,
      "VoxelResolution": 256,
      "WindingThreshold": 0.5,
      "bCloseGaps": false,
      "GapClosingRadius": 0.05,
      "bProjectToSource": true,
      "ProjectionBlendWeight": 0.75,
      "bUseFastPath": true,
      "GeometricDeviation": 0.1
    }
  }
}
```

返回生成结果 JSON：`outputVertexCount`, `outputTriangleCount`, `triangleRatio`, `elapsedSeconds`, `warning`（可选）。

### Inspect（实例）

获取当前碰撞体的完整反馈：自动 6 视角截图 + 统计数据。

```json
{
  "object": "<实例路径>",
  "function": "Inspect"
}
```

返回 JSON 包含：
- `source`: 原始网格统计（vertexCount, triangleCount, boundsSize）
- `collision`: 碰撞体统计（vertexCount, triangleCount, boundsSize, triangleRatio, isGenerated）
- `captures`: 6 张截图路径数组（front, back, left, right, top, perspective）
- `lastGenerateElapsed`: 上次生成耗时

截图保存在 `Saved/PneumaView/<ViewName>/` 下，可通过 Read 工具读取图片。

### Accept（实例）

接受当前碰撞体，保存到原始资产的 ComplexCollisionMesh，关闭编辑器。

```json
{
  "object": "<实例路径>",
  "function": "Accept"
}
```

### Reject（实例）

拒绝，清理临时资产，关闭编辑器。

```json
{
  "object": "<实例路径>",
  "function": "Reject"
}
```

## 参数决策指南

`Open` 返回的分析数据中包含 `suggestedParams`，可直接采纳。以下是手动决策规则：

| 分析数据特征 | 推荐参数 |
|---|---|
| `isClosed=true`, `boundaryEdgeCount=0` | 默认参数即可，Resolution=256 |
| `isClosed=false`, `boundaryEdgeCount>100` | `bWeldBoundaryEdges=true`, `bFillSmallHoles=true` |
| `hasThinWalls=true` | `bThickenThinParts=true` |
| 镂空/网格状结构 | `bCloseGaps=true`, `GapClosingRadius` 根据网格密度调整（0.02-0.1m） |
| 植被/树叶 | 高 `GeometricDeviation`（0.2-0.5），低 `VoxelResolution`（64-128） |
| 大模型（maxDimension > 5m） | 适当降低 Resolution（128-256），提高 GeometricDeviation |
| 小模型（maxDimension < 0.5m） | 提高 Resolution（256-512），降低 GeometricDeviation（0.01-0.05） |

## 参数说明

### Preprocessing
- `bWeldBoundaryEdges`: 焊接重合边界边，改善网格连通性
- `WeldTolerance`: 焊接距离容差（米）
- `bFillSmallHoles`: 填充小孔洞
- `MaxHoleArea`: 最大孔洞面积（平方米）

### Thickening
- `bThickenThinParts`: 为薄壁添加虚拟厚度壳
- `ShellThickness`: 壳厚度（米），未覆盖时自动 = VoxelSize * 1.5

### Voxelization
- `VoxelSizeMode`: `Resolution`（自动）或 `FixedSize`（手动）
- `VoxelResolution`: 最长轴方向的体素数量，越高越精细
- `WindingThreshold`: 内外判定阈值（0.5 = 标准）

### Gap Closing
- `bCloseGaps`: 通过 SDF 形态学闭合填充间隙
- `GapClosingRadius`: 闭合半径（米），小于 2x 此值的间隙会被填充

### Source Projection
- `bProjectToSource`: 将体素化顶点投影回原始表面
- `ProjectionBlendWeight`: 投影混合权重（1.0 = 完全投影）

### Simplification
- `bUseFastPath`: 使用快速预简化通道
- `GeometricDeviation`: 最大几何偏差（米），控制最终网格密度

## 评估标准

- **三角面比率**（triangleRatio）：目标 5-15%，过高说明简化不够，过低可能丢失细节
- **包围盒匹配**：碰撞体包围盒应与原始模型接近
- **截图检查**：从多角度确认无明显穿模、过度简化或不必要的膨胀

## Changelog

- 1.0.0 (2026-03-29): Initial release
