---
name: unreal-live-coding
description: Trigger Live Coding compilation via UCP. Use when the user asks to recompile C++ code, trigger live coding, hot reload C++ changes, or check compilation status in Unreal Engine.
---

# Unreal Live Coding

Trigger and manage Live Coding (runtime C++ recompilation) through UCP's `call` command. The `Compile` function uses **deferred response** — UCP.py will automatically wait for compilation to finish without blocking the UE editor or requiring polling.

**Prerequisite**: The `unreal-client-protocol` skill must be available, the UE editor must be running with UCP enabled, and Live Coding must be enabled in Editor Preferences (Windows only).

## Custom Function Library

### ULiveCodingOperationLibrary

**CDO Path**: `/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary`

| Function | Params | Returns | Description |
|----------|--------|---------|-------------|
| `Compile` | (none) | `FUCPDeferredResponse` | Trigger Live Coding compile. Response is deferred — UCP.py waits until compilation finishes, then receives the result. |
| `IsCompiling` | (none) | `bool` | Whether a Live Coding compile is in progress |
| `IsLiveCodingEnabled` | (none) | `bool` | Whether Live Coding is enabled for the current session |
| `EnableLiveCoding` | `bEnable` (bool) | `bool` | Enable or disable Live Coding for the current session |

#### Deferred Response

`Compile` returns `FUCPDeferredResponse` — a special UCP return type that enables **non-blocking async execution**:

1. UCP.py sends the `Compile` request
2. UE starts compilation and immediately frees the game thread (no blocking)
3. UCP.py keeps the TCP connection open, waiting for the response (no polling needed)
4. When compilation finishes, UE pushes the result back to UCP.py
5. UCP.py receives the result and returns it to the caller

**Important**: Set `UE_TIMEOUT` environment variable to a larger value (e.g. `120`) before calling Compile, as compilation may take longer than the default 30-second timeout:

```powershell
$env:UE_TIMEOUT = "120"
```

#### Examples

**Trigger compilation (waits for completion):**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"Compile"}
```

**Check if currently compiling:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsCompiling"}
```

**Check if Live Coding is enabled:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsLiveCodingEnabled"}
```

**Enable Live Coding:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"EnableLiveCoding","params":{"bEnable":true}}
```

## Common Patterns

### Edit C++ and recompile

After modifying `.cpp` files, trigger Live Coding to apply changes without restarting the editor:

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"Compile"}
```

The response will contain `{"status": "success"}` when compilation succeeds.

### Check availability before compiling

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"IsLiveCodingEnabled"}
```

If false, enable it first:

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__LiveCodingOperationLibrary","function":"EnableLiveCoding","params":{"bEnable":true}}
```

## Live Coding Limitations

- **Windows only** — Live Coding is not available on other platforms
- **Header file changes not supported** — changes to `UCLASS`, `USTRUCT`, `UENUM`, `UFUNCTION` declarations, or any changes in header files require a full editor restart
- **Data layout changes** — modifying struct/class member variables can cause crashes; only function body changes are safe
- **New classes** — adding entirely new C++ classes may require a full rebuild
- **`.cpp` body changes only** — the safest use case is modifying function implementations in `.cpp` files
