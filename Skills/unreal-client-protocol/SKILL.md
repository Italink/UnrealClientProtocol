---
name: unreal-client-protocol
description: Interact with the running Unreal Engine editor via TCP bridge. Use when the user asks to call UFunctions or perform any Unreal Engine operation. This is the transport layer — see domain-specific skills (unreal-object-operation, unreal-material-editing, etc.) for specific operations.
---

# UnrealClientProtocol

Communicate with a running UE editor through the UnrealClientProtocol TCP plugin. UCP exposes a single command — call any UFunction on any UObject via JSON. All functionality is provided through Blueprint Function Libraries that you invoke this way.

## Invocation

When you read this SKILL.md, you already know its absolute path. Replace the filename with `scripts/UCP.py` to get UCP.py's path. For example, if this file is at `X/Skills/unreal-client-protocol/SKILL.md`, then UCP.py is at `X/Skills/unreal-client-protocol/scripts/UCP.py`. **Do NOT search or glob for UCP.py.**

Use PowerShell **here-string** (`@'...'@`) to pipe JSON into UCP.py. This avoids all quote/escape issues:

```powershell
@'
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"FindObjectInstances","params":{"ClassName":"/Script/Engine.World"}}
'@ | python "<path-to-UCP.py>"
```

**IMPORTANT**: The `@'` must be on its own line, and `'@` must also be at the start of its own line. This is PowerShell here-string syntax — the content between them is passed verbatim with zero escaping needed.

**NEVER** use `echo '...'` or `echo "..."` for JSON in PowerShell — quotes and braces will be corrupted.

## Command Format

```json
{"object":"<object_path>","function":"<func_name>","params":{...}}
```

- `object`: Full UObject path — use CDO path for static/library functions, instance path for member methods.
- `function`: The UFunction name exactly as declared in C++.
- `params`: (optional) Map of parameter name -> value. UObject* params accept path strings. Omit `WorldContextObject`.

### Parameter handling

- **Out parameters** (`TArray<AActor*>& OutActors`, etc.) can be omitted from `params` — they are auto-initialized and returned as part of the result after execution.
- **Return values** and all out parameters are serialized together in the response. For example, calling `GetAllActorsOfClass` with only `{"ActorClass":"/Script/Engine.StaticMeshActor"}` returns `{"OutActors":["/Game/Maps/Main.Main:PersistentLevel.Cube_0", ...]}`.
- **UObject\* parameters** accept full object paths as strings. The system resolves them automatically.

## Complex Operation Strategy

When an operation involves predictable multi-step logic (loops, conditionals, bulk modifications), **do NOT issue many individual calls**. Instead:

1. Write a Python script that performs all the steps in one go.
2. Execute it via a single call to `UPythonScriptLibrary::ExecutePythonScript`.

This dramatically reduces tool-call round-trips and gives you the full power of Python for flow control.

## Core Principle — "Knowledge First"

You (the AI) already possess extensive knowledge of the Unreal Engine C++ / Blueprint API. **Always leverage that knowledge to construct commands directly**. Use `DescribeObject` / `DescribeObjectFunction` only when uncertain about project-specific classes.

### Key Rules

1. **Construct from knowledge first.** You know UE APIs. Just call them.
2. **Script for bulk operations.** When you need loops or conditionals, write a Python script and execute it via `ExecutePythonScript`.
3. **WorldContext is auto-injected.** Never pass `WorldContextObject` manually.
4. **Latent functions are not supported.** Functions with `FLatentActionInfo` will be rejected.
5. **Check the `log` field.** If the response contains a `log` array, warnings or errors occurred.
6. **Read error responses carefully.** A failed call returns `{"error":"...", "expected":{...}}` — use it to self-correct.

## Object Path Conventions

| Kind | Pattern | Example |
|------|---------|---------|
| Static/CDO | `/Script/<Module>.Default__<Class>` | `/Script/Engine.Default__KismetSystemLibrary` |
| Instance | `/Game/Maps/<Level>.<Level>:PersistentLevel.<Actor>` | `/Game/Maps/Main.Main:PersistentLevel.BP_Hero_C_0` |
| Class (for find) | `/Script/<Module>.<Class>` | `/Script/Engine.StaticMeshActor` |

**CDO class name convention:** Drop the `U` or `A` prefix. `UKismetSystemLibrary` -> `Default__KismetSystemLibrary`.

## Available Blueprint Function Libraries

UCP provides several function libraries. Each has its own Skill for detailed documentation:

| Library | CDO Path | Skill | Purpose |
|---------|----------|-------|---------|
| `UObjectOperationLibrary` | `/Script/UnrealClientProtocol.Default__ObjectOperationLibrary` | `unreal-object-operation` | Object property R/W, reflection, instance search |
| `UObjectEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__ObjectEditorOperationLibrary` | `unreal-object-operation` | Undo/Redo transactions |
| `UAssetEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary` | `unreal-asset-operation` | Get AssetRegistry instance for asset queries |
| `UNodeCodeEditingLibrary` | `/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary` | `unreal-material-editing` / `unreal-blueprint-editing` | Unified node graph read/write (Material, Blueprint) |
| `UActorEditorOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__ActorEditorOperationLibrary` | `unreal-actor-editing` | Actor operations (placeholder) |
| `UPIEOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary` | `unreal-pie-control` | Start/stop/pause PIE and SIE sessions |
| `ULiveCodingOperationLibrary` | `/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary` | `unreal-live-coding` | Live Coding compile (deferred response) |

## Response Format

- **Success**: Returns the result value directly, no wrapper.
- **Failure**: Returns `{"error":"...", "expected":{...}}` where `expected` contains the function signature.
- **Transaction ID**: Every response includes an `"id"` field (e.g. `"UCP-A1B2C3D4"`), automatically generated by the plugin. This ID is also the Undo transaction description — record it for safe undo (see `unreal-object-operation` skill).
- **Deferred Response**: Functions returning `FUCPDeferredResponse` use async execution — UCP.py keeps the TCP connection open and receives the result when the operation completes. No polling needed. Set `UE_TIMEOUT` to a larger value for long operations (e.g. `$env:UE_TIMEOUT = "120"` for Live Coding compile).
- **Log**: If warnings/errors occurred, a `log` field (string array) is appended.
- **Log level**: Add `"log_level":"all"` to any request to capture all log levels (default captures Warning+). Options: `"all"`, `"log"`, `"display"`, `"warning"` (default), `"error"`.
