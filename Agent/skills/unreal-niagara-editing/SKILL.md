---
name: unreal-niagara-editing
description: >-
  通过 UCP 编辑 Niagara 系统、发射器和脚本。当用户要求创建/修改粒子特效、管理发射器、配置模块、
  设置模块输入、编辑 Niagara 脚本图或任何 Niagara VFX 操作时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-nodecode-common
  tags: "nodecode niagara vfx particle emitter module"
  updated: "2026-03-29"
---

# Niagara 编辑

Niagara 编辑使用三条互补路径：**NiagaraOperationLibrary** 用于结构操作，**NodeCode** 用于图/属性读写，**ObjectOperationLibrary** 用于详细的 UObject 属性访问。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## API 概览

### NiagaraOperationLibrary（结构操作）

CDO：`/Script/UnrealClientProtocolEditor.Default__NiagaraOperationLibrary`

UObject 路径参数（System、Emitter、Script 等）由 UCP 自动解析 — 传入对象路径字符串，UCP 将其转换为 UObject 指针。

| 函数 | 参数 | 说明 |
|------|------|------|
| `AddEmitter` | `System`(路径), `SourceEmitter`(路径), `Name` | 向系统添加发射器 |
| `RemoveEmitter` | `System`(路径), `EmitterName` | 从系统移除发射器 |
| `ReorderEmitter` | `System`(路径), `EmitterName`, `NewIndex` | 重排发射器顺序 |
| `AddUserParameter` | `System`(路径), `ParamName`, `TypeName`, `DefaultValue` | 添加 User 参数 |
| `RemoveUserParameter` | `System`(路径), `ParamName` | 移除 User 参数 |
| `AddModule` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleScript`(路径), `Index` | 向栈添加模块 |
| `RemoveModule` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid` | 移除模块 |
| `MoveModule` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `NewIndex` | 重排模块顺序 |
| `SetModuleEnabled` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `bEnabled` | 启用/禁用模块 |
| `GetModules` | `SystemOrEmitter`(路径), `ScriptUsage` | 列出模块（名称/guid/启用状态/脚本路径） |
| `SetModuleInputValue` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `Value` | 设置输入为字面值（快速迭代） |
| `SetModuleInputBinding` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `LinkedParamName` | 将输入绑定到参数 |
| `SetModuleInputDynamicInput` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `DynamicInputScript`(路径) | 设置动态输入 |
| `ResetModuleInput` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid`, `InputName` | 重置为默认值 |
| `GetModuleInputs` | `SystemOrEmitter`(路径), `ScriptUsage`, `ModuleNodeGuid` | 列出输入（名称/类型/模式/值） |
| `AddRenderer` | `Emitter`(路径), `RendererClassName` | 添加渲染器 |
| `RemoveRenderer` | `Emitter`(路径), `RendererIndex` | 移除渲染器 |
| `MoveRenderer` | `Emitter`(路径), `RendererIndex`, `NewIndex` | 重排渲染器顺序 |
| `AddEventHandler` | `Emitter`(路径) | 添加事件处理器 |
| `RemoveEventHandler` | `Emitter`(路径), `UsageIdString` | 移除事件处理器 |
| `AddSimulationStage` | `Emitter`(路径) | 添加模拟阶段 |
| `RemoveSimulationStage` | `Emitter`(路径), `StageIdString` | 移除模拟阶段 |
| `MoveSimulationStage` | `Emitter`(路径), `StageIdString`, `NewIndex` | 重排模拟阶段顺序 |
| `AddGraphParameter` | `Script`(路径), `ParamName`, `TypeName` | 向脚本图添加参数 |
| `RemoveGraphParameter` | `Script`(路径), `ParamName` | 移除参数 |
| `RenameGraphParameter` | `Script`(路径), `OldName`, `NewName` | 重命名参数 |
| `CreateScratchPadScript` | `SystemOrEmitter`(路径), `ScriptName`, `Usage`, `ModuleUsageBitmask` | 创建 ScratchPad（本地模块）脚本 |

### NodeCode（图 + 属性读写）

CDO：`/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| 函数 | 参数 | 说明 |
|------|------|------|
| `Outline` | `AssetPath` | 列出可用区段 |
| `ReadGraph` | `AssetPath`, `Section` | 读取区段内容为文本 |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | 写回区段内容 |

## 区段模型

### 系统资产（UNiagaraSystem）

| 区段 | 类型 | 说明 |
|------|------|------|
| `[SystemProperties]` | 属性 | 预热、包围盒、确定性、可扩展性、LWC、渲染默认值 |
| `[Emitters]` | 属性 | 发射器列表（名称、启用状态、ObjectPath） |
| `[UserParameters]` | 属性 | User. 命名空间的暴露参数 |
| `[ScratchPadScripts]` | 属性 | 本地模块脚本（名称、Usage、ModuleUsageBitmask、ObjectPath） |
| `[SystemSpawn]` | 图 | 系统生成阶段模块链 |
| `[SystemUpdate]` | 图 | 系统更新阶段模块链 |

### 发射器对象（UNiagaraEmitter，通过 [Emitters] 的 ObjectPath 访问）

| 区段 | 类型 | 说明 |
|------|------|------|
| `[EmitterProperties]` | 属性 | SimTarget、bLocalSpace、包围盒、分配、继承 |
| `[ParticleAttributes]` | 属性（只读） | 编译后的粒子属性列表 |
| `[Renderers]` | 属性 | 渲染器列表（类名、ObjectPath） |
| `[EventHandlers]` | 属性 | 事件处理器配置 |
| `[SimulationStages]` | 属性 | 模拟阶段列表 |
| `[EmitterSpawn]` | 图 | 发射器生成阶段 |
| `[EmitterUpdate]` | 图 | 发射器更新阶段 |
| `[ParticleSpawn]` | 图 | 粒子生成阶段 |
| `[ParticleUpdate]` | 图 | 粒子更新阶段 |
| `[ParticleEvent:EventId]` | 图 | 事件处理器图 |
| `[SimulationStage:StageId]` | 图 | 自定义模拟阶段图 |

### 独立脚本（UNiagaraScript）

| 区段 | 类型 | 说明 |
|------|------|------|
| `[Module]` | 图 | 模块脚本图 |
| `[Function]` | 图 | 函数脚本图 |
| `[DynamicInput]` | 图 | 动态输入脚本图 |

## ScriptUsage 值

`ScriptUsage` 参数使用以下字符串：

`SystemSpawn`、`SystemUpdate`、`EmitterSpawn`、`EmitterUpdate`、`ParticleSpawn`、`ParticleUpdate`、`ParticleEvent`、`SimulationStage`、`Module`、`Function`、`DynamicInput`

## 参数命名空间

| 命名空间 | 作用域 | 可写入位置 |
|----------|--------|-----------|
| `System.` | 系统级属性 | 系统阶段 |
| `Emitter.` | 发射器级属性 | 发射器阶段 |
| `Particles.` | 粒子属性 | 粒子阶段 |
| `Module.` | 模块输入参数 | 模块内部 |
| `User.` | 用户暴露参数 | 运行时可设置 |
| `Engine.` | 引擎值（DeltaTime 等） | 只读 |
| `Local.Module.` | 模块局部临时变量 | 模块内部 |
| `Transient.` | 跨模块临时变量 | 当前阶段 |
| `Constants.` | 快速迭代参数 | 编辑器设置值 |

## 模块输入设置方式

模块输入可通过四种方式设置：

1. **字面值**（`SetModuleInputValue`）— 存储为快速迭代参数 `Constants.EmitterName.ModuleName.InputName`，无图节点
2. **绑定**（`SetModuleInputBinding`）— ParameterMapGet 读取另一个参数（如 `User.MyParam`、`Emitter.SpawnRate`）
3. **动态输入**（`SetModuleInputDynamicInput`）— DynamicInput 脚本计算值
4. **数据接口** — NiagaraNodeInput 提供数据接口对象
5. **默认值** — 模块内置默认值，无覆盖

## 工作流

### 探索系统

```json
// 1. 获取系统结构
{"object":"...NodeCodeEditingLibrary","function":"Outline","params":{"AssetPath":"/Game/FX/NS_Fountain"}}

// 2. 读取发射器列表
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Fountain","Section":"Emitters"}}

// 3. 深入某个发射器（使用步骤 2 的 ObjectPath）
{"object":"...NodeCodeEditingLibrary","function":"Outline","params":{"AssetPath":"/Game/FX/NS_Fountain.NS_Fountain:Fountain"}}
```

### 修改模块栈

```json
// 添加模块
{"object":"...NiagaraOperationLibrary","function":"AddModule","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleAssetPath":"/Niagara/Modules/AddVelocity","Index":1}}

// 设置模块输入为值
{"object":"...NiagaraOperationLibrary","function":"SetModuleInputValue","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleGuid":"...","InputName":"Velocity","Value":"(X=0,Y=0,Z=100)"}}

// 将模块输入绑定到 User 参数
{"object":"...NiagaraOperationLibrary","function":"SetModuleInputBinding","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleGuid":"...","InputName":"Velocity","LinkedParamName":"User.InitialVelocity"}}
```

### 编辑模块脚本内部图

```
ReadGraph("/Game/FX/Modules/MyModule", "Module")
→ 节点图的文本表示

WriteGraph("/Game/FX/Modules/MyModule", "Module", <修改后的文本>)
→ Diff 结果
```

### 修改渲染器属性

```json
// 获取渲染器列表
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"...Fountain","Section":"Renderers"}}

// 通过反射读取渲染器详细属性
{"object":"...ObjectOperationLibrary","function":"DescribeObject","params":{"ObjectPath":"...SpriteRenderer_0"}}

// 设置渲染器属性
{"object":"...ObjectOperationLibrary","function":"SetObjectProperty","params":{"ObjectPath":"...SpriteRenderer_0","PropertyName":"Material","JsonValue":"\"/Game/Materials/M_Particle\""}}
```

## 图区段中的节点类型

| 编码名称 | UClass | 说明 |
|----------|--------|------|
| `FunctionCall:<ScriptPath>` | UNiagaraNodeFunctionCall | 模块/函数调用（资产模块） |
| `FunctionCall:ScratchPad:<Name>` | UNiagaraNodeFunctionCall | ScratchPad（本地模块）引用 |
| `Assignment` | UNiagaraNodeAssignment | 内联赋值 |
| `CustomHlsl` | UNiagaraNodeCustomHlsl | 自定义 HLSL 代码 |
| `ParameterMapGet` | UNiagaraNodeParameterMapGet | 从参数映射读取 |
| `ParameterMapSet` | UNiagaraNodeParameterMapSet | 写入参数映射 |
| `NiagaraOp` | UNiagaraNodeOp | 数学运算（OpName 属性） |
| `NiagaraNodeInput` | UNiagaraNodeInput | 输入节点 |
| `NiagaraNodeOutput` | UNiagaraNodeOutput | 输出节点 |

### ScratchPad（本地模块）脚本操作

ScratchPad 脚本是嵌入在系统或发射器中的内联模块/函数/动态输入脚本。它们出现在 `[ScratchPadScripts]` 中，在图中以 `FunctionCall:ScratchPad:<Name>` 引用。

```json
// 1. 从源系统读取 ScratchPad 列表
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Source","Section":"ScratchPadScripts"}}

// 2. 在目标系统中创建对应的 ScratchPad 脚本
{"object":"...NiagaraOperationLibrary","function":"CreateScratchPadScript","params":{"SystemOrEmitter":"/Game/FX/NS_Target","ScriptName":"MyModule","Usage":"Module","ModuleUsageBitmask":56}}

// 3. 读取源 ScratchPad 内部图（使用步骤 1 的 ObjectPath）
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Source.NS_Source:MyModule","Section":"Module"}}

// 4. 写入目标 ScratchPad（使用步骤 2 的 ObjectPath）
{"object":"...NodeCodeEditingLibrary","function":"WriteGraph","params":{"AssetPath":"/Game/FX/NS_Target.NS_Target:MyModule","Section":"Module","GraphText":"..."}}

// 5. 现在写入阶段图 — FunctionCall:ScratchPad:MyModule 自动解析到目标系统的脚本
```

## 重要说明

- **GUID 保留**：使用 WriteGraph 编辑图时，保留已有节点的 `#guid` 值以确保正确的 diff 匹配。
- **发射器是嵌入的**：系统内的发射器是副本而非引用。通过 `[Emitters]` 区段的 ObjectPath 访问。
- **ParticleAttributes 是只读的**：它们由编译后的脚本派生。要添加/移除粒子属性，添加/移除写入 `Particles.` 命名空间的模块。
- **模块栈变更优先使用 OperationLibrary**：使用 AddModule/RemoveModule/SetModuleInput 而非 WriteGraph 操作系统/发射器阶段图，因为这些 API 正确处理快速迭代参数和 Override 节点管理。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)