---
name: unreal-live-coding
description: >-
  通过 UCP 触发 Live Coding 编译。当用户要求重新编译 C++ 代码、触发 Live Coding、
  热重载 C++ 变更或检查编译状态时使用。
metadata:
  version: "1.0.0"
  layer: composite
  parent: unreal-client-protocol
  tags: "live-coding compile hot-reload cpp"
  updated: "2026-03-29"
---

# Live Coding

通过 UCP 的 `call` 命令触发和管理 Live Coding（运行时 C++ 重编译）。`Compile` 函数使用**延迟响应** — UCP.py 会自动等待编译完成，无需阻塞 UE 编辑器或轮询。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

此外需已在编辑器首选项中启用 Live Coding（仅 Windows）。

## 自定义函数库

### ULiveCodingOperationLibrary

**CDO 路径**：`/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary`

| 函数 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `Compile` | （无） | `FUCPDeferredResponse` | 触发 Live Coding 编译。响应是延迟的 — UCP.py 等待编译完成后接收结果。 |
| `IsCompiling` | （无） | `bool` | 是否有 Live Coding 编译进行中 |
| `IsLiveCodingEnabled` | （无） | `bool` | 当前会话是否启用了 Live Coding |
| `EnableLiveCoding` | `bEnable`（bool） | `bool` | 启用或禁用当前会话的 Live Coding |

#### 延迟响应

`Compile` 返回 `FUCPDeferredResponse` — UCP 的特殊返回类型，支持**非阻塞异步执行**：

1. UCP.py 发送 `Compile` 请求
2. UE 开始编译并立即释放游戏线程（无阻塞）
3. UCP.py 保持 TCP 连接等待响应（无需轮询）
4. 编译完成后，UE 将结果推送回 UCP.py
5. UCP.py 接收结果并返回给调用者

**重要**：调用 Compile 前将 `UE_TIMEOUT` 环境变量设为更大的值（如 `120`），因为编译可能超过默认的 30 秒超时：

```powershell
$env:UE_TIMEOUT = "120"
```

#### 示例

**触发编译（等待完成）：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"Compile"}
```

**检查是否正在编译：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsCompiling"}
```

**检查 Live Coding 是否启用：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsLiveCodingEnabled"}
```

**启用 Live Coding：**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"EnableLiveCoding","params":{"bEnable":true}}
```

## 常用模式

### 编辑 C++ 后重编译

修改 `.cpp` 文件后，触发 Live Coding 应用变更而无需重启编辑器：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"Compile"}
```

编译成功时响应包含 `{"status": "success"}`。

### 编译前检查可用性

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsLiveCodingEnabled"}
```

如果为 false，先启用：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"EnableLiveCoding","params":{"bEnable":true}}
```

## Live Coding 限制

- **仅 Windows** — 其他平台不可用
- **不支持头文件变更** — `UCLASS`、`USTRUCT`、`UENUM`、`UFUNCTION` 声明的变更或任何头文件变更需要完全重启编辑器
- **数据布局变更** — 修改结构体/类成员变量可能导致崩溃；仅函数体变更是安全的
- **新类** — 添加全新 C++ 类可能需要完整重建
- **仅 `.cpp` 函数体变更** — 最安全的用例是修改 `.cpp` 文件中的函数实现

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)