---
name: unreal-material-editing
description: Edit UE material node graphs and properties via text (ReadGraph/WriteGraph). Use when the user asks to add, remove, or rewire material expression nodes, or change material properties like ShadingModel or BlendMode.
---

# Material Editing

Material editing covers both **material properties** (ShadingModel, BlendMode, etc.) via the `[Properties]` section, and **node graph** (expression nodes and connections) via the `[Material]` section. Both use the same unified API.

**Prerequisite**: UE editor running with UCP plugin enabled.

## API

CDO: `/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| Function | Params | Description |
|----------|--------|-------------|
| `Outline` | `AssetPath` | Returns available sections (Properties, Material, Composite:Name) |
| `ReadGraph` | `AssetPath`, `Section` | Returns text. Empty Section = all. `"Material"` = main graph only. |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | Overwrite section. Auto-recompiles, relayouts, refreshes editor. |

## Material Properties

Use the `[Properties]` section to read/write material-level settings:

```
[Properties]
ShadingModel: MSM_DefaultLit
BlendMode: BLEND_Opaque
TwoSided: true
OpacityMaskClipValue: 0.333
```

Common properties:

| Property | Example Values |
|----------|---------------|
| `ShadingModel` | `MSM_DefaultLit`, `MSM_Unlit`, `MSM_Subsurface`, `MSM_ClearCoat` |
| `BlendMode` | `BLEND_Opaque`, `BLEND_Masked`, `BLEND_Translucent`, `BLEND_Additive` |
| `MaterialDomain` | `MD_Surface`, `MD_DeferredDecal`, `MD_LightFunction`, `MD_PostProcess`, `MD_UI` |
| `TwoSided` | `true`, `false` |

Read with `ReadGraph(AssetPath, "Properties")`, write with `WriteGraph(AssetPath, "Properties", text)`.

## Section Model

| Section | Description |
|---------|-------------|
| `[Properties]` | Material-level properties (ShadingModel, BlendMode, etc.) |
| `[Material]` | Complete main graph — all non-composite nodes, read/write as one unit |
| `[Composite:Name]` | Composite subgraph (physically isolated) |

The `[Material]` section contains the **entire main graph**. Output pins are expressed as graph output connections: `> RGB -> [BaseColor]`.

## Text Format

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

Note how `Time` and `WorldPosition` — source-only nodes with no inputs — both have `> ->` lines. Without these lines they would be deleted as orphans.

### Nodes

- `N_<id>`: node reference ID. Two forms:
  - **Existing nodes**: `N_<base62>` — a 22-character Base62-encoded GUID (e.g. `N_4kVm2xRp8bNw3yLq9dJs`). ReadGraph always outputs this form. **Preserve these IDs when writing back** — they are the node's stable identity.
  - **New nodes**: `N_new<int>` — temporary ID for nodes you are creating (e.g. `N_new0`, `N_new1`). The system assigns a real GUID after WriteGraph.
- `<ClassName>`: UMaterialExpression class name (e.g. `MaterialExpressionMultiply`, `MaterialExpressionConstant3Vector`)
- `{...}`: non-default properties, single line

### Connections

**Connections are declared on the source (output) node only.** Each `>` line under a node declares where that node's output goes. There is no "reverse" declaration — if a node has no `>` lines, it has no outgoing connections.

```
  > OutputPin -> N_<target>.InputPin    # named output to named input
  > OutputPin -> [GraphOutput]          # named output to material output
  > -> N_<target>.InputPin              # single-output node to named input
  > -> N_<target>                       # single-output to single-input (both omitted)
```

**CRITICAL — Orphan cleanup:** After WriteGraph, any node not reachable from the material output pins is **automatically deleted** as an orphan. This means:

- **Every node must be part of a connected chain that reaches a `[GraphOutput]`.** If you create a node but forget to write its `> ->` output connections, it will be silently deleted.
- **Source-only nodes** (nodes with no inputs, only outputs — e.g. `Time`, `ScreenPosition`, `ViewSize`, `TexCoord`, `WorldPosition`, `CameraPositionWS`, `VertexColor`, constants) are especially prone to this. They **must** have `> ->` lines connecting their output to downstream nodes.

### Multi-Output Nodes

Some nodes have multiple named outputs. Use the output name before `->`:

| Node | Outputs |
|------|---------|
| `MaterialExpressionScreenPosition` | `ViewportUV` (float2, index 0), `PixelPosition` (float2, index 1) |
| `MaterialExpressionTextureSample` | `RGB`, `R`, `G`, `B`, `A`, `RGBA` |
| `MaterialExpressionWorldPosition` | (single output, omit name) |
| `MaterialExpressionViewSize` | (single output float2, omit name) |

Example — ScreenPosition with named output:
```
N_4kVm2xRp8bNw3yLq9dJs MaterialExpressionScreenPosition
  > ViewportUV -> N_7tHn5cWs1fPx6zMr0gKu.A
```

### Common Input Pin Names

| Node Type | Input Pins |
|-----------|-----------|
| Single-input nodes (`Sine`, `Cosine`, `Tangent`, `Abs`, `OneMinus`, `Frac`, `Floor`, `Ceil`, `Saturate`, `SquareRoot`, `Length`, `Normalize`) | `Input` (can be omitted) |
| `ComponentMask` | `Input` (can be omitted) |
| Binary math (`Multiply`, `Add`, `Subtract`, `Divide`) | `A`, `B` |
| `Power` | `Base`, `Exponent` |
| `DotProduct`, `CrossProduct`, `Distance` | `A`, `B` |
| `AppendVector` | `A`, `B` |
| `LinearInterpolate` | `A`, `B`, `Alpha` |
| `Clamp` | `Input`, `Min`, `Max` |
| `If` | `A`, `B`, `AGreaterThanB`, `AEqualsB`, `ALessThanB` |
| `Arctangent2Fast` | `Y`, `X` |

### Graph Outputs

Material output pins are expressed as `[PinName]`:

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

### Common Expression Classes

**Source-only nodes** (no inputs — must always have `> ->` output lines or they will be deleted as orphans):

| ClassName | Output | Description |
|-----------|--------|-------------|
| `MaterialExpressionConstant` | float1 | Float constant (property: `R`) |
| `MaterialExpressionConstant2Vector` | float2 | Vector2 constant |
| `MaterialExpressionConstant3Vector` | float3 | Vector3 constant (property: `Constant:(R=,G=,B=,A=)`) |
| `MaterialExpressionConstant4Vector` | float4 | Vector4 constant |
| `MaterialExpressionScalarParameter` | float1 | Float parameter |
| `MaterialExpressionVectorParameter` | float3 | Vector parameter |
| `MaterialExpressionTexCoord` | float2 | Texture coordinates |
| `MaterialExpressionTime` | float1 | Time value |
| `MaterialExpressionScreenPosition` | float2 × 2 | Outputs: `ViewportUV`, `PixelPosition` |
| `MaterialExpressionViewSize` | float2 | Viewport resolution in pixels |
| `MaterialExpressionWorldPosition` | float3 | World position |
| `MaterialExpressionVertexColor` | float4 | Vertex color (outputs: R, G, B, A, RGB) |
| `MaterialExpressionCameraPositionWS` | float3 | Camera position |
| `MaterialExpressionTextureObject` | Texture | Texture reference |

**Texture sampling:**

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionTextureSample` | Sample a texture (outputs: RGB, R, G, B, A, RGBA) |
| `MaterialExpressionTextureSampleParameter2D` | Texture parameter |

**Math — binary:**

| ClassName | Inputs | Unconnected-default properties | Description |
|-----------|--------|-------------------------------|-------------|
| `MaterialExpressionMultiply` | `A`, `B` | `ConstA`, `ConstB` (float) | A * B |
| `MaterialExpressionAdd` | `A`, `B` | `ConstA`, `ConstB` (float) | A + B |
| `MaterialExpressionSubtract` | `A`, `B` | `ConstA`, `ConstB` (float) | A - B |
| `MaterialExpressionDivide` | `A`, `B` | `ConstA`, `ConstB` (float) | A / B |
| `MaterialExpressionPower` | `Base`, `Exponent` | `ConstExponent` (float) | Base ^ Exponent |
| `MaterialExpressionDotProduct` | `A`, `B` | *(none)* | Dot(A, B) → float1 |
| `MaterialExpressionCrossProduct` | `A`, `B` | *(none)* | Cross(A, B) → float3 |
| `MaterialExpressionDistance` | `A`, `B` | *(none)* | Distance(A, B) → float1 |
| `MaterialExpressionAppendVector` | `A`, `B` | *(none — both inputs required)* | Append(A, B) — concatenates dimensions. **Max output is float4.** float1+float1→float2, float2+float2→float4. float3+float2 or larger **FAILS**. |

`ConstA`/`ConstB` are **only available on the four basic arithmetic nodes** (Multiply, Add, Subtract, Divide). They provide a scalar fallback when the corresponding input pin is unconnected. All other binary nodes require explicit input connections — use a `MaterialExpressionConstant` node to supply fixed values.

**Math — unary** (input: `Input`, can be omitted in connection):

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionOneMinus` | 1 - X |
| `MaterialExpressionAbs` | Absolute value |
| `MaterialExpressionNormalize` | Normalize → same dimension |
| `MaterialExpressionLength` | Length → float1 |
| `MaterialExpressionSquareRoot` | sqrt |
| `MaterialExpressionFrac` | Fractional part |
| `MaterialExpressionFloor` | Floor |
| `MaterialExpressionCeil` | Ceiling |
| `MaterialExpressionSaturate` | Clamp to 0-1 |

**Trig** (input: `Input`; `Period` property: default 1 maps input range [0,1] to one full cycle. **WARNING: `Period:0` is BROKEN** — it injects a float4 multiply that corrupts dimensions. For raw radians use `Period:6.283185` (= 2π), which maps [0, 2π] to one full cycle):

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionSine` | sin |
| `MaterialExpressionCosine` | cos |
| `MaterialExpressionTangent` | tan |
| `MaterialExpressionArctangent2Fast` | atan2(Y, X) — inputs: `Y`, `X` |

**Interpolation & logic:**

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionLinearInterpolate` | Lerp(A, B, Alpha) |
| `MaterialExpressionClamp` | Clamp(Input, Min, Max) |
| `MaterialExpressionIf` | If(A, B, AGreaterThanB, AEqualsB, ALessThanB) |
| `MaterialExpressionStaticSwitchParameter` | Static bool parameter |

**Channel operations:**

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionComponentMask` | Mask channels (properties: R, G, B, A booleans) |

**Other:**

| ClassName | Description |
|-----------|-------------|
| `MaterialExpressionPanner` | UV panning |
| `MaterialExpressionTransform` | Transform vector between spaces |
| `MaterialExpressionCustom` | Custom HLSL code |
| `MaterialExpressionMaterialFunctionCall` | Call a material function |
| `MaterialExpressionSetMaterialAttributes` | Set material attributes |

## Custom HLSL

For `MaterialExpressionCustom`, the `Code` property contains HLSL and `InputNames` defines custom inputs:

```
N_new0 MaterialExpressionCustom {Code:"float3 result = Input1 * Input2;\nreturn result;", OutputType:CMOT_Float3, InputNames:["A","B"]}
```

### Defining Functions via Struct Wrapping

UE's Custom node **does support function definitions** — wrap them inside a `struct`. Direct top-level function definitions (`float Foo(){...}`) are rejected by the compiler, but struct member functions work:

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
} Helpers;              // <-- instance name after closing brace

return Helpers.Compute(UV, Time);   // call via instance
```

**Key rules:**
1. Define a `struct` with all helper functions as member methods.
2. **Declare an instance** immediately after the closing brace: `} InstanceName;`
3. Call functions via the instance: `InstanceName.FunctionName(args)`.
4. Struct methods can call other methods in the same struct freely.
5. Multiple structs are allowed — each must have its own instance name.
6. **Prefer struct functions over `#define` macros** — they support proper scoping, recursion-free multi-line logic, and the compiler gives better error messages.

## Material Instances

Material instance parameter editing uses `SetObjectProperty` from the `unreal-object-operation` skill, not NodeCode. NodeCode is for editing the **parent material's** node graph.

## Workflow

1. **Outline** — see what sections exist
2. **ReadGraph("Properties")** — check current material settings
3. **ReadGraph("Material")** — get the full node graph
4. **Modify** — edit properties and/or nodes
5. **WriteGraph("Properties", text)** — update settings
6. **WriteGraph("Material", text)** — update graph (auto-recompiles)

## Key Rules

1. **Preserve node IDs** — existing nodes have `N_<base62>` IDs that encode their GUID. Always keep them unchanged when writing back. Use `N_new<int>` only for genuinely new nodes.
2. **`[Material]` is the complete main graph** — no per-output-pin splitting.
3. **ReadGraph before WriteGraph** — always read first.
4. All operations support **Undo** (Ctrl+Z).
5. **Incremental diff** — only changed nodes/connections are modified.
6. **Every node needs output connections** — nodes without `> ->` lines that aren't reachable from material outputs will be deleted as orphans. This is especially important for source-only nodes (constants, Time, ScreenPosition, ViewSize, etc.).
7. **Connections are output-side only** — you declare where a node's output goes by writing `> ->` lines under it. There is no way to declare an incoming connection on the target side.
8. **Track dimensions through the graph** — UE material nodes have strict type rules. Binary math nodes broadcast (`float1 op floatN → floatN`). `AppendVector` concatenates dimensions with a **max of float4**. `ComponentMask` reduces dimensions. `DotProduct` and `Length` always output float1. When building complex graphs, mentally track the dimension at each node to avoid overflow at `AppendVector`. See the **Dimension Rules** section below.
9. **Verify all node IDs before writing** — every `N_<id>` referenced in `> ->` connection lines must be defined as a node line in the same graph. Referencing an undefined ID will silently drop that connection.
10. **Pin name rule** — single-input nodes (`Sine`, `Cosine`, `Abs`, `Saturate`, `OneMinus`, `ComponentMask`, `Length`, etc.) use `Input` as their pin name or omit it entirely. NEVER use `.A` or `.B` for single-input nodes — those names only exist on binary nodes (`Multiply`, `Add`, `DotProduct`, etc.).

## Dimension Rules

UE material compiler enforces strict dimension rules. Track the float-width through your node chain:

| Operation | Output dimension |
|-----------|-----------------|
| `Multiply/Add/Subtract/Divide(floatN, floatN)` | floatN |
| `Multiply/Add/Subtract/Divide(float1, floatN)` | floatN (broadcast) |
| `AppendVector(floatA, floatB)` | float(A+B), **max float4** |
| `ComponentMask` with K channels enabled | floatK |
| `DotProduct(floatN, floatN)` | float1 |
| `CrossProduct(float3, float3)` | float3 |
| `Length(floatN)` | float1 |
| `Distance(floatN, floatN)` | float1 |
| `Normalize(floatN)` | floatN |
| `Sine/Cosine/Tangent(floatN)` | floatN |
| `Abs/OneMinus/Saturate/Frac/Floor/Ceil(floatN)` | floatN |
| `Arctangent2Fast(float1, float1)` | float1 |

Common pitfall: passing a float2 through `Multiply(float2, float2)` still yields float2, but then `AppendVector(float2, float2)` yields float4. Another `AppendVector` with that float4 will **fail to compile**.

## Error Handling

- Check `diff` object in response for changes applied.
- Unknown expression class: `"Unknown expression class: ..."`.
- Pin not found: `"Input 'X' not found on N_... (ClassName). Available: [...]"`.
