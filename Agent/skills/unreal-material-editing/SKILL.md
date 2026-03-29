---
name: unreal-material-editing
description: >-
  通过文本化读写（ReadGraph/WriteGraph）编辑 UE 材质节点图和属性。当用户要求添加、删除或重连材质表达式节点，
  或修改 ShadingModel、BlendMode 等材质属性时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-nodecode-common
  tags: "nodecode material graph-editing expression"
  updated: "2026-03-29"
---

# 材质编辑

材质编辑涵盖两部分：通过 `[Properties]` 区段读写**材质属性**（ShadingModel、BlendMode 等），以及通过 `[Material]` 区段读写**节点图**（表达式节点和连接）。两者使用同一套统一 API。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## API

CDO：`/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| 函数 | 参数 | 说明 |
|------|------|------|
| `Outline` | `AssetPath` | 返回可用区段（Properties、Material、Composite:Name） |
| `ReadGraph` | `AssetPath`, `Section` | 返回文本。Section 为空 = 全部。`"Material"` = 仅主图。 |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | 覆写区段。自动重编译、重布局、刷新编辑器。 |

## 材质属性

使用 `[Properties]` 区段读写材质级别设置：

```
[Properties]
ShadingModel: MSM_DefaultLit
BlendMode: BLEND_Opaque
TwoSided: true
OpacityMaskClipValue: 0.333
```

常用属性：

| 属性 | 示例值 |
|------|--------|
| `ShadingModel` | `MSM_DefaultLit`、`MSM_Unlit`、`MSM_Subsurface`、`MSM_ClearCoat` |
| `BlendMode` | `BLEND_Opaque`、`BLEND_Masked`、`BLEND_Translucent`、`BLEND_Additive` |
| `MaterialDomain` | `MD_Surface`、`MD_DeferredDecal`、`MD_LightFunction`、`MD_PostProcess`、`MD_UI` |
| `TwoSided` | `true`、`false` |

用 `ReadGraph(AssetPath, "Properties")` 读取，用 `WriteGraph(AssetPath, "Properties", text)` 写入。

## 区段模型

| 区段 | 说明 |
|------|------|
| `[Properties]` | 材质级别属性（ShadingModel、BlendMode 等） |
| `[Material]` | 完整主图 — 所有非 Composite 节点，作为一个整体读写 |
| `[Composite:Name]` | Composite 子图（物理隔离） |

`[Material]` 区段包含**整个主图**。输出引脚表示为图输出连接：`> RGB -> [BaseColor]`。

## 文本格式

```
[Material]

N_4kVm2xRp8bNw3yLq9dJs MaterialExpressionTextureSample {Texture:"/Game/Textures/T_Wood_BC"}
  > RGB -> [BaseColor]
  > A -> N_7tHn5cWs1fPx6zMr0gKu.A

N_9aEo3jXv2hQy8bFt4iLw MaterialExpressionScalarParameter {ParameterName:"Roughness", DefaultValue:0.5}
  > -> N_7tHn5cWs1fPx6zMr0gKu.B

N_2dGq6lZx4jSa0cHv8kNy MaterialExpressionVectorParameter {ParameterName:"EmissiveColor", DefaultValue:{"R":1,"G":0.5,"B":0}}
  > -> N_5fIr7mBz3kTb1eJw9lOa.A

N_7tHn5cWs1fPx6zMr0gKu MaterialExpressionMultiply
  > -> [Roughness]

N_5fIr7mBz3kTb1eJw9lOa MaterialExpressionMultiply
  > -> [EmissiveColor]

N_0hKt8nDc5lUd2gLx6mPb MaterialExpressionConstant {R:5.0}
  > -> N_5fIr7mBz3kTb1eJw9lOa.B

N_3jMv0pFe7nWf4iNz8oCd MaterialExpressionTextureSample {Texture:"/Game/Textures/T_Wood_N"}
  > -> [Normal]

N_6lOx2rHg9pYh5kPb1qEf MaterialExpressionTime
  > -> N_8nQz4tJi0rAj7mRd3sFg.A

N_8nQz4tJi0rAj7mRd3sFg MaterialExpressionSine {Period:6.283185}
  > -> N_1pSb6vLk2tCl9oTf5uGh.B

N_1pSb6vLk2tCl9oTf5uGh MaterialExpressionMultiply
  > -> [WorldPositionOffset]

N_4rUd8xNm3vEn0qVh7wIj MaterialExpressionWorldPosition
  > -> N_1pSb6vLk2tCl9oTf5uGh.A
```

注意 `Time` 和 `WorldPosition` — 只有输出没有输入的源节点 — 都有 `> ->` 行。没有这些行它们会被当作孤立节点删除。

### 节点

- `N_<id>`：节点引用 ID。两种形式：
  - **已有节点**：`N_<base62>` — 22 字符 Base62 编码的 GUID（如 `N_4kVm2xRp8bNw3yLq9dJs`）。ReadGraph 始终输出此形式。**写回时保持这些 ID 不变** — 它们是节点的稳定身份标识。
  - **新节点**：`N_new<int>` — 你创建的节点的临时 ID（如 `N_new0`、`N_new1`）。WriteGraph 后系统分配真实 GUID。
- `<ClassName>`：UMaterialExpression 类名（如 `MaterialExpressionMultiply`、`MaterialExpressionConstant3Vector`）
- `{...}`：非默认属性，单行

### 连接

**连接仅在源（输出）节点上声明。** 每个节点下的 `>` 行声明该节点的输出去向。没有"反向"声明 — 如果节点没有 `>` 行，它就没有输出连接。

```
  > OutputPin -> N_<target>.InputPin    # 命名输出到命名输入
  > OutputPin -> [GraphOutput]          # 命名输出到材质输出
  > -> N_<target>.InputPin              # 单输出节点到命名输入
  > -> N_<target>                       # 单输出到单输入（都省略）
```

**关键 — 孤立节点清理：** WriteGraph 后，任何无法从材质输出引脚到达的节点都会被**自动删除**。这意味着：

- **每个节点都必须是连接到 `[GraphOutput]` 的链的一部分。** 如果创建了节点但忘记写 `> ->` 输出连接，它会被静默删除。
- **源节点**（只有输出没有输入的节点 — 如 `Time`、`ScreenPosition`、`ViewSize`、`TexCoord`、`WorldPosition`、`CameraPositionWS`、`VertexColor`、常量）尤其容易出问题。它们**必须**有 `> ->` 行将输出连接到下游节点。

### 多输出节点

部分节点有多个命名输出。在 `->` 前使用输出名：

| 节点 | 输出 |
|------|------|
| `MaterialExpressionScreenPosition` | `ViewportUV`（float2，索引 0）、`PixelPosition`（float2，索引 1） |
| `MaterialExpressionTextureSample` | `RGB`、`R`、`G`、`B`、`A`、`RGBA` |
| `MaterialExpressionWorldPosition` | （单输出，省略名称） |
| `MaterialExpressionViewSize` | （单输出 float2，省略名称） |

示例 — ScreenPosition 的命名输出：
```
N_4kVm2xRp8bNw3yLq9dJs MaterialExpressionScreenPosition
  > ViewportUV -> N_7tHn5cWs1fPx6zMr0gKu.A
```

### 常用输入引脚名

| 节点类型 | 输入引脚 |
|----------|----------|
| 单输入节点（`Sine`、`Cosine`、`Tangent`、`Abs`、`OneMinus`、`Frac`、`Floor`、`Ceil`、`Saturate`、`SquareRoot`、`Length`、`Normalize`） | `Input`（可省略） |
| `ComponentMask` | `Input`（可省略） |
| 二元数学（`Multiply`、`Add`、`Subtract`、`Divide`） | `A`、`B` |
| `Power` | `Base`、`Exponent` |
| `DotProduct`、`CrossProduct`、`Distance` | `A`、`B` |
| `AppendVector` | `A`、`B` |
| `LinearInterpolate` | `A`、`B`、`Alpha` |
| `Clamp` | `Input`、`Min`、`Max` |
| `If` | `A`、`B`、`AGreaterThanB`、`AEqualsB`、`ALessThanB` |
| `Arctangent2Fast` | `Y`、`X` |

### 图输出

材质输出引脚表示为 `[PinName]`：

```
  > RGB -> [BaseColor]
  > -> [Roughness]
  > -> [Normal]
  > -> [EmissiveColor]
  > -> [Opacity]
  > -> [OpacityMask]
  > -> [Metallic]
  > -> [WorldPositionOffset]
```

### 常用表达式类

**源节点**（无输入 — 必须始终有 `> ->` 输出行，否则会被当作孤立节点删除）：

| 类名 | 输出 | 说明 |
|------|------|------|
| `MaterialExpressionConstant` | float1 | 浮点常量（属性：`R`） |
| `MaterialExpressionConstant2Vector` | float2 | Vector2 常量 |
| `MaterialExpressionConstant3Vector` | float3 | Vector3 常量（属性：`Constant:(R=,G=,B=,A=)`） |
| `MaterialExpressionConstant4Vector` | float4 | Vector4 常量 |
| `MaterialExpressionScalarParameter` | float1 | 浮点参数 |
| `MaterialExpressionVectorParameter` | float3 | 向量参数 |
| `MaterialExpressionTexCoord` | float2 | 纹理坐标 |
| `MaterialExpressionTime` | float1 | 时间值 |
| `MaterialExpressionScreenPosition` | float2 × 2 | 输出：`ViewportUV`、`PixelPosition` |
| `MaterialExpressionViewSize` | float2 | 视口分辨率（像素） |
| `MaterialExpressionWorldPosition` | float3 | 世界位置 |
| `MaterialExpressionVertexColor` | float4 | 顶点色（输出：R、G、B、A、RGB） |
| `MaterialExpressionCameraPositionWS` | float3 | 相机位置 |
| `MaterialExpressionTextureObject` | Texture | 纹理引用 |

**纹理采样：**

| 类名 | 说明 |
|------|------|
| `MaterialExpressionTextureSample` | 采样纹理（输出：RGB、R、G、B、A、RGBA） |
| `MaterialExpressionTextureSampleParameter2D` | 纹理参数 |

**数学 — 二元：**

| 类名 | 输入 | 未连接时的默认属性 | 说明 |
|------|------|-------------------|------|
| `MaterialExpressionMultiply` | `A`、`B` | `ConstA`、`ConstB`（float） | A * B |
| `MaterialExpressionAdd` | `A`、`B` | `ConstA`、`ConstB`（float） | A + B |
| `MaterialExpressionSubtract` | `A`、`B` | `ConstA`、`ConstB`（float） | A - B |
| `MaterialExpressionDivide` | `A`、`B` | `ConstA`、`ConstB`（float） | A / B |
| `MaterialExpressionPower` | `Base`、`Exponent` | `ConstExponent`（float） | Base ^ Exponent |
| `MaterialExpressionDotProduct` | `A`、`B` | *（无）* | Dot(A, B) → float1 |
| `MaterialExpressionCrossProduct` | `A`、`B` | *（无）* | Cross(A, B) → float3 |
| `MaterialExpressionDistance` | `A`、`B` | *（无）* | Distance(A, B) → float1 |
| `MaterialExpressionAppendVector` | `A`、`B` | *（无 — 两个输入都必须连接）* | Append(A, B) — 拼接维度。**最大输出 float4。** float1+float1→float2，float2+float2→float4。float3+float2 或更大**会失败**。 |

`ConstA`/`ConstB` **仅在四个基本算术节点**（Multiply、Add、Subtract、Divide）上可用。当对应输入引脚未连接时提供标量回退值。其他二元节点都需要显式输入连接 — 使用 `MaterialExpressionConstant` 节点提供固定值。

**数学 — 一元**（输入：`Input`，连接中可省略）：

| 类名 | 说明 |
|------|------|
| `MaterialExpressionOneMinus` | 1 - X |
| `MaterialExpressionAbs` | 绝对值 |
| `MaterialExpressionNormalize` | 归一化 → 维度不变 |
| `MaterialExpressionLength` | 长度 → float1 |
| `MaterialExpressionSquareRoot` | 平方根 |
| `MaterialExpressionFrac` | 小数部分 |
| `MaterialExpressionFloor` | 向下取整 |
| `MaterialExpressionCeil` | 向上取整 |
| `MaterialExpressionSaturate` | 钳制到 0-1 |

**三角函数**（输入：`Input`；`Period` 属性：默认 1 将输入范围 [0,1] 映射为一个完整周期。**警告：`Period:0` 会出错** — 它会注入一个 float4 乘法破坏维度。对于原始弧度使用 `Period:6.283185`（= 2π），将 [0, 2π] 映射为一个完整周期）：

| 类名 | 说明 |
|------|------|
| `MaterialExpressionSine` | sin |
| `MaterialExpressionCosine` | cos |
| `MaterialExpressionTangent` | tan |
| `MaterialExpressionArctangent2Fast` | atan2(Y, X) — 输入：`Y`、`X` |

**插值与逻辑：**

| 类名 | 说明 |
|------|------|
| `MaterialExpressionLinearInterpolate` | Lerp(A, B, Alpha) |
| `MaterialExpressionClamp` | Clamp(Input, Min, Max) |
| `MaterialExpressionIf` | If(A, B, AGreaterThanB, AEqualsB, ALessThanB) |
| `MaterialExpressionStaticSwitchParameter` | 静态布尔参数 |

**通道操作：**

| 类名 | 说明 |
|------|------|
| `MaterialExpressionComponentMask` | 通道遮罩（属性：R、G、B、A 布尔值） |

**其他：**

| 类名 | 说明 |
|------|------|
| `MaterialExpressionPanner` | UV 平移 |
| `MaterialExpressionTransform` | 空间变换向量 |
| `MaterialExpressionCustom` | 自定义 HLSL 代码 |
| `MaterialExpressionMaterialFunctionCall` | 调用材质函数 |
| `MaterialExpressionSetMaterialAttributes` | 设置材质属性 |

## 自定义 HLSL

`MaterialExpressionCustom` 的 `Code` 属性包含 HLSL，`InputNames` 定义自定义输入：

```
N_new0 MaterialExpressionCustom {Code:"float3 result = Input1 * Input2;\nreturn result;", OutputType:CMOT_Float3, InputNames:["A","B"]}
```

### 通过 Struct 封装定义函数

UE 的 Custom 节点**支持函数定义** — 将它们封装在 `struct` 中。直接的顶层函数定义（`float Foo(){...}`）会被编译器拒绝，但 struct 成员函数可以：

```hlsl
struct MyHelpers
{
    float Hash(float t)
    {
        return frac(sin(t * 613.2) * 614.8);
    }

    float2 Hash2D(float t)
    {
        return float2(
            frac(sin(t * 213.3) * 314.8) - 0.5,
            frac(sin(t * 591.1) * 647.2) - 0.5
        );
    }

    float3 Compute(float2 uv, float time)
    {
        float h = Hash(time);
        float2 offset = Hash2D(time);
        return float3(uv + offset, h);
    }
} Helpers;              // <-- 闭合花括号后的实例名

return Helpers.Compute(UV, Time);   // 通过实例调用
```

**关键规则：**
1. 定义一个 `struct`，将所有辅助函数作为成员方法。
2. **在闭合花括号后声明实例**：`} InstanceName;`
3. 通过实例调用函数：`InstanceName.FunctionName(args)`。
4. struct 方法可自由调用同一 struct 中的其他方法。
5. 允许多个 struct — 每个必须有自己的实例名。
6. **优先使用 struct 函数而非 `#define` 宏** — 它们支持正确的作用域、无递归的多行逻辑，且编译器给出更好的错误信息。

## 材质实例

材质实例参数编辑使用 `unreal-object-operation` Skill 的 `SetObjectProperty`，而非 NodeCode。NodeCode 用于编辑**父材质的**节点图。

## 工作流

1. **Outline** — 查看有哪些区段
2. **ReadGraph("Properties")** — 检查当前材质设置
3. **ReadGraph("Material")** — 获取完整节点图
4. **修改** — 编辑属性和/或节点
5. **WriteGraph("Properties", text)** — 更新设置
6. **WriteGraph("Material", text)** — 更新图（自动重编译）

## 关键规则

1. **保持节点 ID 不变** — 已有节点有 `N_<base62>` ID 编码其 GUID。写回时始终保持不变。仅对真正的新节点使用 `N_new<int>`。
2. **`[Material]` 是完整主图** — 不按输出引脚拆分。
3. **先 ReadGraph 再 WriteGraph** — 始终先读取。
4. 所有操作支持 **Undo**（Ctrl+Z）。
5. **增量 diff** — 仅修改实际变化的节点/连接。
6. **每个节点都需要输出连接** — 没有 `> ->` 行且无法从材质输出到达的节点会被当作孤立节点删除。源节点（常量、Time、ScreenPosition、ViewSize 等）尤其需要注意。
7. **连接仅在输出侧声明** — 你通过在节点下写 `> ->` 行来声明输出去向。无法在目标侧声明输入连接。
8. **跟踪图中的维度** — UE 材质节点有严格的类型规则。二元数学节点广播（`float1 op floatN → floatN`）。`AppendVector` 拼接维度且**最大 float4**。`ComponentMask` 减少维度。`DotProduct` 和 `Length` 始终输出 float1。构建复杂图时，在每个节点处心算维度以避免 `AppendVector` 溢出。见下方**维度规则**。
9. **写入前验证所有节点 ID** — `> ->` 连接行中引用的每个 `N_<id>` 都必须在同一图中定义为节点行。引用未定义的 ID 会静默丢弃该连接。
10. **引脚名规则** — 单输入节点（`Sine`、`Cosine`、`Abs`、`Saturate`、`OneMinus`、`ComponentMask`、`Length` 等）使用 `Input` 作为引脚名或完全省略。绝不要对单输入节点使用 `.A` 或 `.B` — 这些名称仅存在于二元节点（`Multiply`、`Add`、`DotProduct` 等）上。

## 维度规则

UE 材质编译器强制执行严格的维度规则。跟踪节点链中的 float 宽度：

| 操作 | 输出维度 |
|------|----------|
| `Multiply/Add/Subtract/Divide(floatN, floatN)` | floatN |
| `Multiply/Add/Subtract/Divide(float1, floatN)` | floatN（广播） |
| `AppendVector(floatA, floatB)` | float(A+B)，**最大 float4** |
| `ComponentMask` 启用 K 个通道 | floatK |
| `DotProduct(floatN, floatN)` | float1 |
| `CrossProduct(float3, float3)` | float3 |
| `Length(floatN)` | float1 |
| `Distance(floatN, floatN)` | float1 |
| `Normalize(floatN)` | floatN |
| `Sine/Cosine/Tangent(floatN)` | floatN |
| `Abs/OneMinus/Saturate/Frac/Floor/Ceil(floatN)` | floatN |
| `Arctangent2Fast(float1, float1)` | float1 |

常见陷阱：将 float2 通过 `Multiply(float2, float2)` 仍得 float2，但 `AppendVector(float2, float2)` 得 float4。再对该 float4 做 `AppendVector` 会**编译失败**。

## 错误处理

- 检查响应中的 `diff` 对象了解应用的变更。
- 未知表达式类：`"Unknown expression class: ..."`。
- 引脚未找到：`"Input 'X' not found on N_... (ClassName). Available: [...]"`。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)