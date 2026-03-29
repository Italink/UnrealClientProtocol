---
name: unreal-large-blueprint-analysis
description: >-
  系统化分析和转译大型 UE 蓝图（10+ 函数）为 C++ 或其他目标。当用户要求将蓝图转为 C++、
  审计复杂蓝图或理解大型蓝图的完整逻辑时使用。
metadata:
  version: "1.0.0"
  layer: composite
  parent: unreal-blueprint-editing
  tags: "blueprint analysis cpp translation audit"
  updated: "2026-03-29"
---

# 大型蓝图分析

系统化策略，用于读取、理解和转译大型蓝图（10+ 区段、100+ 节点）。基于 `unreal-blueprint-editing` 和 `unreal-object-operation` Skill。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## 为什么需要本 Skill

大型蓝图可能有 40+ 区段和 1000+ 节点。仅 EventGraph 的单次 ReadGraph 就可能返回 100KB+ 文本。朴素方法会失败，因为：

- 一次读取所有区段会超出上下文窗口限制
- 跳过区段凭通用知识"猜测"会产生不准确的代码
- 无结构的读取会遗漏函数间的依赖关系

## 阶段 1 — 清点

在读取任何图内容之前，先获取完整结构。

### 步骤 1.1：列出所有区段

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary","function":"Outline","params":{"AssetPath":"<asset_path>"}}
```

记录完整列表。将每个区段分类：

| 类别 | 示例 | 优先级 |
|------|------|--------|
| 入口点 | EventGraph、Event:Construct、Event:Tick | 最先读取 |
| 核心逻辑 | Function:Initialize、Function:CaptureGrid | 其次读取 |
| 辅助函数 | Macro:CheckAndCreateRT、Macro:DeriveAxes | 按需读取 |
| 仅 UI | Macro:UpdateProgress、Macro:CollapsedOrVisible | 最后读取 |

### 步骤 1.2：读取蓝图变量

使用 `unreal-object-operation` Skill 的 `DescribeObject` 获取蓝图 CDO 上的所有 UPROPERTY 变量。这揭示了完整的状态模型 — 蓝图使用的每个变量。

```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"DescribeObject","params":{"ObjectPath":"<blueprint_default_object_path>"}}
```

蓝图类 `MyBlueprint_C`（包路径 `/Game/BP/MyBlueprint`）的 CDO 路径为：
`/Game/BP/MyBlueprint.Default__MyBlueprint_C`

### 步骤 1.3：构建区段依赖图

在读取图内容之前，从区段名推断依赖关系：

- `Function:Initialize` 可能调用 `Function:SetupRT`、`Function:SetCaptureMesh` 等
- `EventGraph` 可能调用大部分函数并响应 UI 事件
- 宏被函数调用 — 先读调用者，遇到宏时再读

## 阶段 2 — 系统化读取

**规则：按依赖顺序读取区段，而非字母顺序。完整处理每个区段后再进入下一个。**

### 读取策略

1. **逐个读取区段** — 保持输出可控
2. **从入口点开始** — 先 EventGraph，再 Construct/PreConstruct
3. **跟随调用链** — 当区段调用 `Function:Foo` 时，将 `Function:Foo` 加入待读队列
4. **每个区段用子 Agent 做伪代码转译** — 见下方

### 用子 Agent 做伪代码转译

**每读一个区段，启动一个子 Agent 将 NodeCode 转译为伪代码。** 这对大型蓝图至关重要，因为：

- 50 个节点的函数的原始 NodeCode 轻松超过 5KB — 会挤占你的推理上下文
- 子 Agent 在隔离环境中处理 NodeCode，返回紧凑的伪代码摘要
- 你在主上下文中只保留伪代码，保持对全局的关注

**子 Agent 提示模板：**

> 将以下蓝图 NodeCode 转译为伪代码。从入口节点追踪执行流，将所有数据依赖解析为内联表达式。
>
> ```
> （在此粘贴 ReadGraph 输出）
> ```
>
> 返回：
> 1. 伪代码（每条语句一行，非显而易见的逻辑加注释）
> 2. 摘要：此区段做什么、读写哪些变量、调用哪些函数

**每个区段的工作流：**

1. ReadGraph → 获取 NodeCode
2. 启动子 Agent 处理 NodeCode → 获取伪代码
3. 在主上下文中记录伪代码摘要
4. 进入下一个区段

### 读取模板

逐个读取区段：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"<path>","Section":"Function:Initialize"}}
```

### 处理大输出

如果单个区段的 ReadGraph 输出超过约 50KB（如 200+ 节点的 EventGraph）：

1. 单独读取（不与其他区段批量处理）
2. 如果 Shell 输出写入文件，分块读取
3. 重点提取：**节点类型、函数调用、变量读写和连接拓扑**
4. 对于 EventGraph，识别所有**事件处理器**（Event:*、K2Node_ComponentBoundEvent）作为独立逻辑块

### 伪代码转译

**关键：读取每个区段后，立即将 NodeCode 转译为伪代码。** 这是最重要的步骤 — NodeCode 描述结构而非逻辑。你必须重建控制流和数据流才能理解区段实际做什么。

1. 找到入口节点（FunctionEntry、Event、CustomEvent）
2. 沿 `then`/`execute` 执行链构建语句顺序
3. 在每个节点处，通过数据链向后追溯解析数据输入
4. 将数学/工具链折叠为内联表达式
5. Branch → `if/else`，ForEachLoop → `for`，Sequence → 顺序块

### 区段摘要格式

转译每个区段后，记录如下摘要：

```
## Function:Initialize
- 伪代码：
    if Orthographic:
        OrthoWidth = ObjectRadius * 2
        ProjectionType = Orthographic
    else:
        FOVAngle = Clamp(CameraFOV, 10, 170)
        ProjectionType = Perspective
    SceneCapture.TextureTarget = CreateRT(...)
    SetupShowOnlyList(SceneCapture, TargetMeshes)
    DynamicMesh.SetMaterial(0, CreateMID(BaseMaterial))
    ResetAllCaptureModes()
- 调用：SetupRTAndSaveList、SetCaptureMeshAndSizeInfo、SetupOctahedronLayout、ClearRenderTargets、CreateAndSetupMIDs、ResetAllCaptureModes
- 读取：SceneCaptureComponent2D、StaticMeshComponent、DynamicMeshComponent、CaptureUsingGBuffer、Orthographic、CameraDistance、ObjectRadius
- 写入：OrthoWidth、FOVAngle、ProjectionType、TextureTarget、CurrentMapIndex
- 节点数：47
```

## 阶段 3 — 转译

将蓝图转为 C++ 时，遵循以下规则：

### 变量映射

1. 所有蓝图变量 → UObject 上的 UPROPERTY 成员或分组到设置/状态结构体
2. 蓝图枚举（UserDefinedEnum）→ C++ UENUM，使用**不同名称**避免冲突
3. 蓝图结构体（UserDefinedStruct）→ C++ USTRUCT，使用**不同名称**
4. 函数中的局部变量 → C++ 局部变量

### 函数映射

| 蓝图构造 | C++ 等价物 |
|----------|-----------|
| Function | 成员函数 |
| Macro | 私有辅助函数（内联逻辑） |
| CustomEvent | 成员函数（通过委托或直接调用） |
| Event:Construct / Event:Tick | 重写虚函数 |
| K2Node_ComponentBoundEvent | 初始化中绑定委托 |
| Delay / SetTimerByEvent | FTimerManager::SetTimer |
| ForEachLoop 宏 | 范围 for 循环 |
| Sequence 节点 | 顺序语句 |
| Branch 节点 | if/else |
| Select 节点 | 三元运算或 switch |
| SwitchOnName/Enum | switch 语句 |

### 逐节点转译

对区段中每个节点直接转译：

| 节点文本 | C++ 代码 |
|----------|---------|
| `CallFunction:KismetMathLibrary.Multiply_VectorFloat` | `FVector Result = Vec * Scalar;` |
| `CallFunction:KismetRenderingLibrary.DrawMaterialToRenderTarget` | `UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RT, MID);` |
| `CallFunction:KismetMaterialLibrary.SetVectorParameterValue` | `UKismetMaterialLibrary::SetVectorParameterValue(World, MPC, Name, Value);` |
| `VariableGet:MyVar` | `MyVar`（成员访问） |
| `VariableSet:MyVar` | `MyVar = Value;` |
| `K2Node_IfThenElse` | `if (Condition) { ... } else { ... }` |
| `K2Node_ForEachLoop` | `for (auto& Elem : Array) { ... }` |
| `K2Node_DynamicCast` | `if (auto* Casted = Cast<TargetClass>(Obj)) { ... }` |
| `K2Node_Self` | `this` |

### 执行流重建

连接定义执行顺序。通过以下方式重建 C++ 控制流：

1. 找到入口节点（FunctionEntry、Event、CustomEvent）
2. 沿 `then`/`execute` 执行引脚构建语句序列
3. 在 Branch 节点处创建 if/else 块
4. 在 Sequence 节点处按顺序输出语句（then_0、then_1、...）
5. 数据流引脚变为变量赋值或内联表达式

### 材质 / MPC 引用

当蓝图引用内容资产（材质、MPC、纹理）时：

1. 记录节点属性中引用的每个资产路径（如 `pin.Collection:/Game/MPC/MyMPC.MyMPC`）
2. 在 C++ 中通过 `LoadObject<T>(nullptr, TEXT("/Path/To/Asset"))` 或 `ConstructorHelpers::FObjectFinder` 加载
3. 如果创建独立插件，复制引用的资产并更新所有路径

## 阶段 4 — 验证

### 完整性检查

转译完成后，验证每个区段都已处理：

1. 重新读取 Outline 输出
2. 将每个区段与你的 C++ 实现对照
3. 任何未处理的区段 = 缺失功能

### 编译和测试

1. 使用 UBT 的 `-Module=YourModule` 隔离编译
2. 迭代修复错误
3. 如果 UE 编辑器运行中，使用 UCP 在运行时测试 C++ 代码

## 反模式

| 错误做法 | 为什么会失败 | 正确做法 |
|----------|------------|---------|
| 读 4 个区段就写全部 C++ | 90% 的逻辑是猜测而非转译 | 读取每个区段，逐一转译 |
| 一次批量读取全部 40 个区段 | 输出过大，上下文溢出 | 每次 3-5 个 |
| 跳过宏（"只是辅助函数"） | 宏包含关键逻辑（RT 创建、mip 链、mesh 保存） | 读取每个被调用的宏 |
| 用通用 UE 知识代替读取图 | 实际实现可能与标准模式不同 | 始终先读取，再转译所见 |
| 将 EventGraph 转译为一个整体函数 | EventGraph 有多个独立事件处理器 | 将每个事件处理器拆分为独立 C++ 函数 |

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)