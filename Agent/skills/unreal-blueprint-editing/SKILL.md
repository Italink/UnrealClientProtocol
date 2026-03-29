---
name: unreal-blueprint-editing
description: >-
  通过文本化读写（ReadGraph/WriteGraph）编辑 UE 蓝图节点图。当用户要求添加、删除或重连蓝图节点，
  创建/修改函数、事件或变量时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-nodecode-common
  tags: "nodecode blueprint graph-editing function event"
  updated: "2026-03-29"
---

# 蓝图编辑

通过文本化表示编辑蓝图节点图。如需设置蓝图资产本身的对象属性，使用 `unreal-object-operation` Skill。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## 节点图 API

CDO：`/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| 函数 | 参数 | 说明 |
|------|------|------|
| `Outline` | `AssetPath` | 返回可用区段（EventGraph、Function:Name、Macro:Name、Variables） |
| `ReadGraph` | `AssetPath`, `Section` | 返回文本表示。Section 为空 = 所有区段。 |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | 覆写区段。GraphText 为空 = 删除区段。自动重编译。 |

## 读取策略

**关键：绝不一次读取所有区段。** 蓝图可能有数十个函数/宏。

1. **先 Outline** — 获取结构
2. **识别相关区段** — 根据任务只选需要的
3. **逐个 ReadGraph** — 只读需要的
4. **读更多前先总结** — 判断是否需要更多上下文

## 区段类型

| 区段格式 | 说明 |
|----------|------|
| `EventGraph` | 主事件图 |
| `Function:<Name>` | 函数图 |
| `Macro:<Name>` | 宏图 |
| `Variables` | 蓝图变量定义 |

名称中的空格用下划线替代：`Function:My_Function`。

## 文本格式

```
[Function:CalculateDamage]

N_2kVm4xRp8bNw6yLq0dJs FunctionEntry
  > then -> N_8nQz4tJi0rAj7mRd3sFg.execute

N_9aEo3jXv2hQy8bFt4iLw VariableGet:BaseDamage
  > -> N_7tHn5cWs1fPx6zMr0gKu.A

N_5fIr7mBz3kTb1eJw9lOa VariableGet:DamageMultiplier
  > -> N_7tHn5cWs1fPx6zMr0gKu.B

N_7tHn5cWs1fPx6zMr0gKu CallFunction:KismetMathLibrary.Multiply_FloatFloat
  > ReturnValue -> N_0hKt8nDc5lUd2gLx6mPb.Value

N_0hKt8nDc5lUd2gLx6mPb CallFunction:KismetMathLibrary.FClamp {pin.Min:0, pin.Max:9999}
  > ReturnValue -> N_8nQz4tJi0rAj7mRd3sFg.ReturnValue

N_8nQz4tJi0rAj7mRd3sFg FunctionResult
```

### 节点

- `N_<id>`：节点引用 ID。两种形式：
  - **已有节点**：`N_<base62>` — 22 字符 Base62 编码的 GUID（如 `N_2kVm4xRp8bNw6yLq0dJs`）。ReadGraph 始终输出此形式。**写回时保持这些 ID 不变。**
  - **新节点**：`N_new<int>` — 你创建的节点的临时 ID（如 `N_new0`、`N_new1`）。WriteGraph 后系统分配真实 GUID。
- `<ClassName>`：语义编码（见下方）或原始 UE 类名
- `{...}`：非默认属性，单行。无属性时省略花括号。

### 类名编码

| 文本格式 | UE 节点类型 | 示例 |
|----------|------------|------|
| `CallFunction:<Class>.<Func>` | `UK2Node_CallFunction`（外部） | `CallFunction:KismetSystemLibrary.PrintString` |
| `CallFunction:<Func>` | `UK2Node_CallFunction`（自身） | `CallFunction:MyBlueprintFunction` |
| `VariableGet:<Var>` | `UK2Node_VariableGet` | `VariableGet:Health` |
| `VariableSet:<Var>` | `UK2Node_VariableSet` | `VariableSet:Health` |
| `CustomEvent:<Name>` | `UK2Node_CustomEvent` | `CustomEvent:OnDamageReceived` |
| `Event:<Name>` | `UK2Node_Event` | `Event:ReceiveBeginPlay` |
| `FunctionEntry` | `UK2Node_FunctionEntry` | （自动生成，不可创建/删除） |
| `FunctionResult` | `UK2Node_FunctionResult` | （自动生成，不可创建/删除） |

其他节点类型使用原始类名（如 `K2Node_IfThenElse`、`K2Node_ForEachLoop`）。

### 连接

从所属节点输出的连接，缩进 `>`：

```
  > OutputPin -> N_<target>.InputPin    # 输出到目标
  > OutputPin -> [GraphOutput]          # 输出到图输出
  > -> N_<target>.InputPin              # 单输出节点（省略引脚名）
```

引脚名与 UE 内部 `PinName` 一致。常用执行引脚：`execute`、`then`。

### 属性

属性通过 UE 反射序列化在 `{...}` 中。引脚默认值使用 `pin.<PinName>` 前缀。

示例：`{pin.InString:"Hello World", pin.Duration:5.0}`

## Variables 区段

```
[Variables]
Health: {"PinCategory":"real", "PinSubCategory":"double", "DefaultValue":"100.0"}
bIsAlive: {"PinCategory":"bool", "DefaultValue":"true"}
TargetActor: {"PinCategory":"object", "PinSubCategoryObject":"/Script/Engine.Actor"}
DamageHistory: {"PinCategory":"real", "PinSubCategory":"double", "ContainerType":"Array"}
```

字段直接映射到 `FEdGraphPinType` 反射字段。可选：`"Replicated":true`、`"Category":"MyCategory"`。

`[Variables]` 区段使用**完全覆写**语义：
- 文本中存在但蓝图中不存在的变量会被**创建**。
- 文本中存在且蓝图中已有的变量会被**更新**（类型、默认值、复制标志、分类）。
- 蓝图中存在但**文本中没有**的变量会被**删除**。

**始终先 ReadGraph("Variables")** 并包含所有你想保留的变量。

## 创建 / 删除区段

- **创建函数**：`WriteGraph(AssetPath, "Function:NewFunc", graphText)` — 函数不存在时自动创建
- **删除函数**：`WriteGraph(AssetPath, "Function:OldFunc", "")` — 空文本删除区段
- 宏同理

## 伪代码转译

**关键：ReadGraph 后，修改前先将 NodeCode 转译为伪代码。** NodeCode 描述的是结构（节点、属性、连接），不是逻辑。你必须重建逻辑才能避免破坏它。

### 工作流

1. **ReadGraph** — 获取 NodeCode 文本
2. **转译为伪代码** — 从入口节点追踪执行流，将数据流解析为表达式
3. **推理变更** — 用伪代码理解需要改什么
4. **编辑 NodeCode** — 在充分理解逻辑的基础上进行结构修改
5. **WriteGraph** — 应用

## 关键规则

1. **按需读取，不要一次全读。** 先 Outline，再只 ReadGraph 需要的区段。
2. **读取后转译为伪代码** — 编辑前先理解逻辑。
3. **保持节点 ID 不变** — 已有节点有 `N_<base62>` ID 编码其 GUID。写回时始终保持不变。仅对真正的新节点使用 `N_new<int>`。
4. **先 ReadGraph 再 WriteGraph** — 始终先读取目标区段。
5. **FunctionEntry 和 FunctionResult 不可创建或删除。**
6. 所有操作支持 **Undo**（Ctrl+Z）。
7. **增量 diff** — 仅修改实际变化的节点/连接。

## 错误处理

- 检查响应中的 `diff` 对象：`nodes_added`、`nodes_removed`、`nodes_modified`、`links_added`、`links_removed`。
- 属性错误：`"Property not found: ..."` 或 `"Failed to import ..."`。
- 引脚错误：`"Pin not found: ..."`。
- 不确定时写入后重新读取。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)