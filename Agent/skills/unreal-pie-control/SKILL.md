---
name: unreal-pie-control
description: >-
  通过 UCP 控制 Play In Editor（PIE）和 Simulate In Editor（SIE）会话。当用户要求启动、停止、暂停、恢复 PIE/SIE，
  或查询 PIE 状态和 World 时使用。
metadata:
  version: "1.0.0"
  layer: composite
  parent: unreal-client-protocol
  tags: "pie play-in-editor simulate start stop"
  updated: "2026-03-29"
---

# PIE 控制

通过 UCP 的 `call` 命令控制 Play In Editor（PIE）和 Simulate In Editor（SIE）会话。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## 自定义函数库

### UPIEOperationLibrary

**CDO 路径**：`/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary`

| 函数 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `StartPIE` | （无） | `bool` | 启动 PIE 会话 |
| `StartSimulate` | （无） | `bool` | 启动 SIE 会话 |
| `StopPIE` | （无） | （void） | 停止当前 PIE/SIE 会话 |
| `IsInPIE` | （无） | `bool` | 是否有运行中的 Play 会话 |
| `IsSimulating` | （无） | `bool` | 是否有运行中的 Simulate 会话 |
| `PausePIE` | （无） | `bool` | 暂停所有 PIE World |
| `ResumePIE` | （无） | `bool` | 恢复所有 PIE World |
| `GetPIEWorlds` | （无） | `TArray<UWorld*>` | 获取所有 PIE World 实例 |

#### 示例

**启动 PIE：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartPIE"}
```

**停止 PIE：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StopPIE"}
```

**启动 Simulate：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartSimulate"}
```

**检查 PIE 是否运行中：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"IsInPIE"}
```

**暂停 PIE：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"PausePIE"}
```

**获取 PIE World：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"GetPIEWorlds"}
```

## 引擎内置 API

### UEditorLevelLibrary

**CDO 路径**：`/Script/EditorScriptingUtilities.Default__EditorLevelLibrary`

| 函数 | 说明 |
|------|------|
| `GetPIEWorlds()` | 获取 PIE 会话中的 World（自定义库的替代方案） |

### UGameplayStatics（在 PIE World 上）

**CDO 路径**：`/Script/Engine.Default__GameplayStatics`

PIE 运行时，`UGameplayStatics` 函数自动解析到 PIE World（UCP 的 WorldContext 自动注入优先选择 PIE World）：

| 函数 | 说明 |
|------|------|
| `GetPlayerPawn(WorldContextObject, PlayerIndex)` | 获取 PIE 中的玩家 Pawn |
| `GetPlayerController(WorldContextObject, PlayerIndex)` | 获取 PIE 中的玩家控制器 |
| `GetAllActorsOfClass(WorldContextObject, ActorClass)` | 在 PIE World 中查找 Actor |

## 常用模式

### 启动 PIE 后与 PIE World 交互

启动 Play 会话，然后查询运行中游戏的 Actor（分步调用）：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartPIE"}
```

PIE 启动后，UCP 自动将 WorldContext 解析到 PIE World：

```json
{"object":"/Script/Engine.Default__GameplayStatics","function":"GetPlayerPawn","params":{"PlayerIndex":0}}
```

### 暂停、检查、恢复

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"PausePIE"}
```

暂停时检查游戏状态，然后恢复：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"ResumePIE"}
```

### 启动前检查状态

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"IsInPIE"}
```

如果为 false，启动 PIE。如果为 true，需要时先停止再重启。

## 关键规则

1. **同时只能有一个 PIE/SIE 会话**。启动前检查 `IsInPIE`。
2. **StartPIE/StartSimulate 是异步的** — 它们排队请求，会话在下一个编辑器 tick 启动。依赖 PIE 活跃状态的后续调用应作为单独请求发送。
3. **StopPIE 也是异步的** — 它排队结束请求。
4. **WorldContext 自动解析** — PIE 活跃时，UCP 自动为需要 `WorldContextObject` 的函数使用 PIE World。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)