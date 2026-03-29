---
name: unreal-modeling-primitives
description: GeometryScript 图元创建与建模函数。当用户要求创建 Box、Sphere、Cylinder 等基础形状，或进行拉伸、旋转体、壳体等建模操作时使用。
---

# 建模 — 图元与建模

前置条件：先阅读 `unreal-modeling`。

## 库

| 库 | CDO |
|----|-----|
| 图元 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions` |
| 建模 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshModelingFunctions` |

`TargetMesh` 为 `UDynamicMesh*` 路径字符串。`Debug` 省略。

## MeshPrimitiveFunctions — 主要函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `AppendBox` | `PrimitiveOptions`, `DimensionX/Y/Z`, `StepsX/Y/Z` | 追加长方体 |
| `AppendSphere` | `PrimitiveOptions`, `Radius`, `StepsX`, `StepsY` | 追加球体（经纬细分） |
| `AppendSphereBox` | `PrimitiveOptions`, `Radius`, `Steps`, `Subdivisions` | 追加球体（立方体映射细分） |
| `AppendCylinder` | `PrimitiveOptions`, `Radius`, `Height`, `RadialSteps`, `HeightSteps`, `bCapped` | 追加圆柱 |
| `AppendCapsule` | `PrimitiveOptions`, `Radius`, `LineLength`, `HemisphereSteps`, `CircleSteps` | 追加胶囊体 |
| `AppendCone` | `PrimitiveOptions`, `BaseRadius`, `TopRadius`, `Height`, `RadialSteps`, `HeightSteps`, `bCapped` | 追加圆锥/圆台 |
| `AppendTorus` | `PrimitiveOptions`, `InnerRadius`, `OuterRadius`, `CrossSectionSteps`, `CircleSteps` | 追加环面 |
| `AppendDisc` | `PrimitiveOptions`, `Radius`, `AngleSteps`, `SpokeSteps`, `StartAngle`, `EndAngle`, `HoleRadius` | 追加圆盘/扇形 |
| `AppendRectangleXY` | `PrimitiveOptions`, `DimensionX`, `DimensionY`, `StepsWidth`, `StepsHeight` | 追加 XY 平面矩形 |
| `AppendRoundRectangleXY` | `PrimitiveOptions`, `DimensionX`, `DimensionY`, `CornerRadius`, `StepsWidth`, `StepsHeight`, `RoundSteps` | 追加圆角矩形 |
| `AppendLinearStaircase` | `PrimitiveOptions`, `StepWidth`, `StepHeight`, `StepDepth`, `NumSteps`, `bFloating` | 追加楼梯 |
| `AppendSpiralStaircase` | `PrimitiveOptions`, `StepWidth`, `StepHeight`, `InnerRadius`, `SpiralAngle`, `NumSteps`, `bFloating` | 追加螺旋楼梯 |
| `AppendSweepPolyline` | `PrimitiveOptions`, `PolylineVertices`(FVector2D 数组), `SweepPath`, `bLoop`, `StartScale/EndScale` | 沿路径扫掠多段线截面 |
| `AppendSweepPolygon` | `PrimitiveOptions`, `PolygonVertices`, `SweepPath`, `bLoop`, `bCapped`, `StartScale/EndScale` | 沿路径扫掠多边形截面 |
| `AppendSimpleSweptPolygon` | `PrimitiveOptions`, `PolygonVertices`, `SweepPath`, `bLoop`, `bCapped`, `StartScale/EndScale` | 简化版扫掠 |
| `AppendRevolvePolygon` | `PrimitiveOptions`, `PolygonVertices`, `RevolutionOptions` | 旋转体（绕轴旋转多边形截面） |
| `AppendSimpleExtrudePolygon` | `PrimitiveOptions`, `PolygonVertices`, `Height`, `HeightSteps`, `bCapped` | 简单拉伸多边形 |
| `AppendVoronoiDiagram2D` | `PrimitiveOptions`, `VoronoiSites`(FVector2D 数组), `Bounds`, `BoundsExpand` | 追加 2D Voronoi 图 |
| `AppendDelaunayTriangulation2D` | `PrimitiveOptions`, `Vertices`(FVector2D 数组), `ConstrainedEdges` | 追加 Delaunay 三角化 |
| `AppendBoundingBox` | `TargetMesh`, `Box`, `PrimitiveOptions` | 追加包围盒 mesh |

注意：`AppendSweepPolygon`、`AppendRevolvePolygon`、`AppendSimpleExtrudePolygon` 的 `PolygonVertices` 参数类型为 `FGeometryScriptSimplePolygon`，需要用 `MeshOperationLibrary.MakeSimplePolygon` 桥接。`SweepPath` 参数类型为 `FGeometryScriptPolyPath`，需要用 `MakePolyPath` 桥接。

## MeshModelingFunctions — 主要函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplyMeshOffset` | `TargetMesh`, `Options` | 整体偏移（膨胀/收缩） |
| `ApplyMeshShell` | `TargetMesh`, `Options` | 生成壳体（双面偏移） |
| `ApplyMeshLinearExtrudeFaces` | `TargetMesh`, `Options`, `Selection` | 沿法线线性拉伸选中面 |
| `ApplyMeshOffsetFaces` | `TargetMesh`, `Options`, `Selection` | 偏移选中面 |
| `ApplyMeshInsetOutsetFaces` | `TargetMesh`, `Options`, `Selection` | 内缩/外扩选中面 |
| `ApplyMeshBevelSelection` | `TargetMesh`, `Options`, `Selection` | 倒角选中区域 |
| `ApplyMeshPolygroupBevel` | `TargetMesh`, `Options`, `GroupLayer` | 按 Polygroup 边界倒角 |
| `ApplyMeshDisconnectFaces` | `TargetMesh`, `Selection` | 断开选中面（使其成为独立岛） |
| `ApplyMeshDuplicateFaces` | `TargetMesh`, `Selection` | 复制选中面 |

## 结构体与枚举

### `FGeometryScriptPrimitiveOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `PolygroupMode` | `EGeometryScriptPrimitivePolygroupMode` | `PerFace` | Polygroup 分组方式 |
| `bFlipOrientation` | bool | `false` | 翻转法线方向 |
| `UVMode` | `EGeometryScriptPrimitiveUVMode` | `Uniform` | UV 生成模式 |

### `EGeometryScriptPrimitivePolygroupMode`

| 值 | 说明 |
|----|------|
| `PerFace` | 每个面一个 Polygroup |
| `PerQuad` | 每个四边形一个 Polygroup |
| `PerShape` | 整个形状一个 Polygroup |
| `SingleGroup` | 所有图元共享一个 Polygroup |

### `EGeometryScriptPrimitiveUVMode`

| 值 | 说明 |
|----|------|
| `Uniform` | 均匀 UV |
| `ScaleToFill` | 缩放填满 0-1 |

### `FGeometryScriptRevolveOptions`（用于 `AppendRevolvePolygon`）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `RevolveDegrees` | float | `360` | 旋转角度 |
| `DegreeOffset` | float | `0` | 起始角度偏移 |
| `Steps` | int | `24` | 旋转步数 |
| `bReverseDirection` | bool | `false` | 反向旋转 |
| `bHardNormals` | bool | `false` | 硬边法线 |
| `HardNormalAngle` | float | `0` | 硬边角度阈值 |
| `bProfileAtMidpoint` | bool | `false` | 截面在步数中点 |
| `bFillPartialRevolveEndcaps` | bool | `true` | 非 360° 时封口 |

### `FGeometryScriptMeshOffsetOptions`（用于 `ApplyMeshOffset`）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `OffsetDistance` | float | `1.0` | 偏移距离 |
| `bFixedBoundary` | bool | `false` | 固定边界 |
| `Iterations` | int | `0` | 平滑迭代次数 |
| `SolveSteps` | int | `0` | 求解步数 |
| `SmoothAlpha` | float | `0.1` | 平滑系数 |
| `bReprojectSmooth` | bool | `false` | 平滑后重投影 |
| `BoundaryAlignmentWeight` | float | `10.0` | 边界对齐权重 |

### `FGeometryScriptMeshLinearExtrudeOptions`（用于 `ApplyMeshLinearExtrudeFaces`）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Distance` | float | `1.0` | 拉伸距离 |
| `Direction` | `EGeometryScriptLinearExtrudeDirection` | `FaceNormal` | 拉伸方向 |
| `DirectionVector` | FVector | `(0,0,1)` | 自定义方向（当 Direction=Fixed 时） |
| `bSingleDirection` | bool | `true` | 单向拉伸 |

### `EGeometryScriptLinearExtrudeDirection`

| 值 | 说明 |
|----|------|
| `FaceNormal` | 沿面法线 |
| `SingleDirection` | 沿固定方向 |

## JSON 示例

**追加球体**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions","function":"AppendSphere","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","PrimitiveOptions":{"PolygroupMode":"PerFace","bFlipOrientation":false,"UVMode":"Uniform"},"Radius":50,"StepsX":16,"StepsY":16}}
```

**追加圆柱**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions","function":"AppendCylinder","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","PrimitiveOptions":{},"Radius":25,"Height":100,"RadialSteps":16,"HeightSteps":1,"bCapped":true}}
```

**偏移（膨胀）**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshModelingFunctions","function":"ApplyMeshOffset","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Options":{"OffsetDistance":5.0,"bFixedBoundary":false,"Iterations":3}}}
```

## 发现

对上述 CDO 路径调用 `DescribeObject` 可查看当前引擎版本的完整函数列表和参数签名。
