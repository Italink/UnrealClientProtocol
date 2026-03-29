---
name: unreal-client-protocol
description: >-
  通过 TCP 桥接与运行中的 Unreal Engine 编辑器交互。当用户要求调用 UFunction 或执行任何 Unreal Engine 操作时使用。
metadata:
  version: "1.0.0"
  layer: transport
  tags: "tcp json protocol ucp"
  updated: "2026-03-29"
---

# UnrealClientProtocol

通过 UnrealClientProtocol TCP 插件与运行中的 UE 编辑器通信。UCP 暴露唯一一种命令 — 通过 JSON 在任意 UObject 上调用任意 UFunction。所有功能均通过蓝图函数库提供，你以此方式调用它们。

## 调用方式

读取本 SKILL.md 时你已知其绝对路径，将文件名替换为 `scripts/UCP.py` 即为 UCP.py 的路径。例如本文件位于 `X/Agent/skills/unreal-client-protocol/SKILL.md`，则 UCP.py 位于 `X/Agent/skills/unreal-client-protocol/scripts/UCP.py`。**不要搜索或 glob 查找 UCP.py。**

使用 PowerShell **here-string**（`@'...'@`）将 JSON 管道传入 UCP.py，避免所有引号/转义问题：

```powershell
@'
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"FindObjectInstances","params":{"ClassName":"/Script/Engine.World"}}
'@ | python "<path-to-UCP.py>"
```

**重要**：`@'` 必须独占一行，`'@` 也必须在行首。这是 PowerShell here-string 语法 — 中间的内容原样传递，无需任何转义。

**绝对不要**在 PowerShell 中使用 `echo '...'` 或 `echo "..."` 传递 JSON — 引号和花括号会被破坏。

## 命令格式

```json
{"object":"<object_path>","function":"<func_name>","params":{...}}
```

- `object`：完整 UObject 路径 — 静态/库函数用 CDO 路径，成员方法用实例路径。
- `function`：UFunction 名称，与 C++ 声明完全一致。
- `params`：（可选）参数名 -> 值的映射。UObject* 参数接受路径字符串。省略 `WorldContextObject`。

### 参数处理

- **Out 参数**（`TArray<AActor*>& OutActors` 等）可在 `params` 中省略 — 自动初始化并在执行后作为结果返回。
- **返回值**和所有 out 参数一起序列化在响应中。例如调用 `GetAllActorsOfClass` 只传 `{"ActorClass":"/Script/Engine.StaticMeshActor"}`，返回 `{"OutActors":["/Game/Maps/Main.Main:PersistentLevel.Cube_0", ...]}`。
- **UObject\* 参数**接受完整对象路径字符串，系统自动解析。

## 复杂操作策略

当操作涉及可预知的多步逻辑（循环、条件、批量修改）时，**不要逐条发送调用**。而是：

1. 编写一个 Python 脚本一次性完成所有步骤。
2. 通过单次调用 `UPythonScriptLibrary::ExecutePythonScript` 执行。

这大幅减少工具调用的往返次数，并赋予你 Python 的完整流程控制能力。

## 核心原则 — "知识优先"

你（AI）已经拥有丰富的 Unreal Engine C++ / 蓝图 API 知识。**始终优先利用已有知识直接构建命令**。仅在对项目自定义类不确定时才使用 `DescribeObject` / `DescribeObjectFunction` 探索。

### 关键规则

1. **优先凭知识构建命令。** 你了解 UE API，直接调用即可。
2. **批量操作用脚本。** 需要循环或条件时，编写 Python 脚本并通过 `ExecutePythonScript` 执行。
3. **WorldContext 自动注入。** 永远不要手动传递 `WorldContextObject`。
4. **不支持 Latent 函数。** 包含 `FLatentActionInfo` 的函数会被拒绝。
5. **检查 `log` 字段。** 如果响应中包含 `log` 数组，说明出现了警告或错误。
6. **仔细阅读错误响应。** 失败的调用返回 `{"error":"...", "expected":{...}}` — 据此自纠正。

## 对象路径约定

| 类型 | 格式 | 示例 |
|------|------|------|
| 静态/CDO | `/Script/<Module>.Default__<Class>` | `/Script/Engine.Default__KismetSystemLibrary` |
| 实例 | `/Game/Maps/<Level>.<Level>:PersistentLevel.<Actor>` | `/Game/Maps/Main.Main:PersistentLevel.BP_Hero_C_0` |
| 类（用于查找） | `/Script/<Module>.<Class>` | `/Script/Engine.StaticMeshActor` |

**CDO 类名约定：** 去掉 `U` 或 `A` 前缀。`UKismetSystemLibrary` -> `Default__KismetSystemLibrary`。

## 可用蓝图函数库

UCP 提供多个函数库，每个都有对应的 Skill 提供详细文档：

| 库 | CDO 路径 | Skill | 用途 |
|----|----------|-------|------|
| `UObjectOperationLibrary` | `/Script/UnrealClientProtocol.Default__ObjectOperationLibrary` | `unreal-object-operation` | 对象属性读写、反射、实例查找 |
| `UObjectEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__ObjectEditorOperationLibrary` | `unreal-object-operation` | Undo/Redo 事务 |
| `UAssetEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary` | `unreal-asset-operation` | 获取 AssetRegistry 实例进行资产查询 |
| `UNodeCodeEditingLibrary` | `/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary` | `unreal-material-editing` / `unreal-blueprint-editing` | 统一节点图读写（材质、蓝图） |
| `UActorEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__ActorEditorOperationLibrary` | `unreal-actor-editing` | Actor 操作（占位） |
| `UPIEOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary` | `unreal-pie-control` | 启动/停止/暂停 PIE 和 SIE 会话 |
| `ULiveCodingOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary` | `unreal-live-coding` | Live Coding 编译（延迟响应） |

## 响应格式

- **成功**：直接返回结果值，无包装。
- **失败**：返回 `{"error":"...", "expected":{...}}`，其中 `expected` 包含函数签名。
- **事务 ID**：每个响应包含 `"id"` 字段（如 `"UCP-A1B2C3D4"`），由插件自动生成。此 ID 同时是 Undo 事务描述 — 记录它以便安全撤销（见 `unreal-object-operation` Skill）。
- **延迟响应**：返回 `FUCPDeferredResponse` 的函数使用异步执行 — UCP.py 保持 TCP 连接直到操作完成后接收结果。无需轮询。长时间操作需设置更大的 `UE_TIMEOUT`（如 `$env:UE_TIMEOUT = "120"` 用于 Live Coding 编译）。
- **日志**：如果出现警告/错误，响应中会附加 `log` 字段（字符串数组）。
- **日志级别**：在请求中添加 `"log_level":"all"` 可捕获所有级别的日志（默认捕获 Warning+）。可选值：`"all"`、`"log"`、`"display"`、`"warning"`（默认）、`"error"`。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)