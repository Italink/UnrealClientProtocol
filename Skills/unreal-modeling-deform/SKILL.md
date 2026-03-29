---
name: unreal-modeling-deform
description: GeometryScript 变形与变换操作。当用户要求弯曲、扭曲、平滑、噪声、位移、平移、旋转、缩放网格时使用。
---

# 建模 — 变形与变换

前置条件：先阅读 `unreal-modeling`。

## 库

| 库 | CDO |
|----|-----|
| 变形 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshDeformFunctions` |
| 变换 | `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshTransformFunctions` |

`TargetMesh` 为 `UDynamicMesh*` 路径字符串。`Debug` 省略。需要 `FGeometryScriptMeshSelection` 的函数，用 `MeshOperationLibrary` 桥接。

## 变形函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ApplyBendWarpToMesh` | `TargetMesh`, `Options`, `BendOrientation`(FTransform), `BendAngle`(float), `BendExtent`(float) | 弯曲变形 |
| `ApplyTwistWarpToMesh` | `TargetMesh`, `Options`, `TwistOrientation`, `TwistAngle`, `TwistExtent` | 扭曲变形 |
| `ApplyFlareWarpToMesh` | `TargetMesh`, `Options`, `FlareOrientation`, `FlarePercentX`, `FlarePercentY`, `FlareExtent` | 膨胀/收缩变形 |
| `ApplyMathWarpToMesh` | `TargetMesh`, `WarpOrientation`, `WarpType`, `Options` | 数学函数变形（正弦等） |
| `ApplyIterativeSmoothingToMesh` | `TargetMesh`, `Selection`, `Options` | 迭代平滑（拉普拉斯风格） |
| `ApplyDisplaceFromTextureMap` | `TargetMesh`, `Texture`, `Selection`, `Options`, `UVLayer` | 纹理位移 |
| `ApplyPerlinNoiseToMesh2` | `TargetMesh`, `Selection`, `Options` | Perlin 噪声位移 |
| `ApplyDisplaceFromPerVertexVectors` | `TargetMesh`, `Selection`, `VectorList`, `Magnitude` | 按顶点向量位移 |

## 变换函数

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `TransformMesh` | `TargetMesh`, `Transform`, `bFixOrientationForNegativeScale` | 应用完整变换 |
| `InverseTransformMesh` | `TargetMesh`, `Transform`, `bFixOrientationForNegativeScale` | 应用逆变换 |
| `TranslateMesh` | `TargetMesh`, `Translation`(FVector) | 平移 |
| `RotateMesh` | `TargetMesh`, `Rotation`(FRotator), `RotationOrigin`(FVector) | 旋转 |
| `ScaleMesh` | `TargetMesh`, `Scale`(FVector), `ScaleOrigin`, `bFixOrientationForNegativeScale` | 缩放 |
| `TransformMeshSelection` | `TargetMesh`, `Selection`, `Transform` | 变换选中区域 |
| `TranslateMeshSelection` | `TargetMesh`, `Selection`, `Translation` | 平移选中区域 |
| `RotateMeshSelection` | `TargetMesh`, `Selection`, `Rotation`, `RotationOrigin` | 旋转选中区域 |
| `ScaleMeshSelection` | `TargetMesh`, `Selection`, `Scale`, `ScaleOrigin` | 缩放选中区域 |
| `TranslatePivotToLocation` | `TargetMesh`, `PivotLocation` | 移动枢轴点（等效于平移 mesh 到 -PivotLocation） |

JSON 中 `FTransform`：`{"Rotation":{"X":0,"Y":0,"Z":0,"W":1},"Translation":{"X":0,"Y":0,"Z":0},"Scale3D":{"X":1,"Y":1,"Z":1}}`

JSON 中 `FRotator`：`{"Pitch":0,"Yaw":90,"Roll":0}`

## 结构体与枚举

### `FGeometryScriptBendWarpOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bSymmetricExtents` | bool | `true` | 对称范围 |
| `LowerExtent` | float | `10` | 非对称时的下界 |
| `bBidirectional` | bool | `true` | 双向弯曲 |

`BendAngle`、`BendExtent` 是函数参数，不在 Options 内。

### `FGeometryScriptTwistWarpOptions`

同弯曲：`bSymmetricExtents`、`LowerExtent`、`bBidirectional`。`TwistAngle`、`TwistExtent` 是函数参数。

### `FGeometryScriptFlareWarpOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `bSymmetricExtents` | bool | `true` | 对称范围 |
| `LowerExtent` | float | `10` | 非对称时的下界 |
| `FlareType` | `EGeometryScriptFlareType` | `SinMode` | 膨胀曲线类型 |

### `EGeometryScriptFlareType`

| 值 | 说明 |
|----|------|
| `SinMode` | 正弦曲线 sin(pi*x) |
| `SinSquaredMode` | 正弦平方（更平滑过渡） |
| `TriangleMode` | 分段线性（三角形轮廓） |

### `FGeometryScriptIterativeMeshSmoothingOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `NumIterations` | int | `10` | 迭代次数 |
| `Alpha` | float | `0.2` | 平滑系数 |
| `EmptyBehavior` | `EGeometryScriptEmptySelectionBehavior` | `FullMeshSelection` | 空选择行为 |

### `FGeometryScriptPerlinNoiseOptions`

| 字段 | 类型 | 说明 |
|------|------|------|
| `BaseLayer` | struct | 包含 `Magnitude`(5.0)、`Frequency`(0.25)、`FrequencyShift`(FVector)、`RandomSeed`(0) |
| `bApplyAlongNormal` | bool | 沿法线方向位移 |
| `EmptyBehavior` | enum | 空选择行为 |

### `FGeometryScriptDisplaceFromTextureOptions`

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Magnitude` | float | `1` | 位移幅度 |
| `UVScale` | FVector2D | `(1,1)` | UV 缩放 |
| `UVOffset` | FVector2D | `(0,0)` | UV 偏移 |
| `Center` | float | `0.5` | 中心值（低于此值为负位移） |
| `ImageChannel` | int | `0` | 采样通道 |
| `EmptyBehavior` | enum | `FullMeshSelection` | 空选择行为 |

### `EGeometryScriptEmptySelectionBehavior`

| 值 | 说明 |
|----|------|
| `FullMeshSelection` | 空选择 = 选择全部 |
| `EmptySelection` | 空选择 = 不选择任何东西 |

## JSON 示例

**迭代平滑**（全 mesh，5 次迭代）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshDeformFunctions","function":"ApplyIterativeSmoothingToMesh","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Selection":{},"Options":{"NumIterations":5,"Alpha":0.15,"EmptyBehavior":"FullMeshSelection"}}}
```

**缩放 mesh**（2 倍均匀缩放，原点为中心）：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshTransformFunctions","function":"ScaleMesh","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Scale":{"X":2,"Y":2,"Z":2},"ScaleOrigin":{"X":0,"Y":0,"Z":0},"bFixOrientationForNegativeScale":true}}
```

**Perlin 噪声位移**：

```json
{"object":"/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshDeformFunctions","function":"ApplyPerlinNoiseToMesh2","params":{"TargetMesh":"/Engine/Transient.DynamicMesh_0","Selection":{},"Options":{"BaseLayer":{"Magnitude":10,"Frequency":0.1,"FrequencyShift":{"X":0,"Y":0,"Z":0},"RandomSeed":42},"bApplyAlongNormal":true,"EmptyBehavior":"FullMeshSelection"}}}
```

## 发现

对上述 CDO 路径调用 `DescribeObject` 可查看完整函数列表和参数签名。
