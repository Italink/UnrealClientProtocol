---
name: unreal-pie-control
description: Control Play In Editor (PIE) and Simulate In Editor (SIE) sessions via UCP. Use when the user asks to start, stop, pause, resume PIE/SIE, or query PIE state and worlds.
---

# Unreal PIE Control

Control Play In Editor (PIE) and Simulate In Editor (SIE) sessions through UCP's `call` command.

**Prerequisite**: The `unreal-client-protocol` skill must be available and the UE editor must be running with the UCP plugin enabled.

## Custom Function Library

### UPIEOperationLibrary

**CDO Path**: `/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary`

| Function | Params | Returns | Description |
|----------|--------|---------|-------------|
| `StartPIE` | (none) | `bool` | Start a Play In Editor session |
| `StartSimulate` | (none) | `bool` | Start a Simulate In Editor session |
| `StopPIE` | (none) | (void) | Stop the current PIE/SIE session |
| `IsInPIE` | (none) | `bool` | Whether a play session is in progress |
| `IsSimulating` | (none) | `bool` | Whether a simulate session is in progress |
| `PausePIE` | (none) | `bool` | Pause all PIE worlds |
| `ResumePIE` | (none) | `bool` | Resume all PIE worlds |
| `GetPIEWorlds` | (none) | `TArray<UWorld*>` | Get all PIE world instances |

#### Examples

**Start PIE:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartPIE"}
```

**Stop PIE:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StopPIE"}
```

**Start Simulate:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartSimulate"}
```

**Check if PIE is running:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"IsInPIE"}
```

**Pause PIE:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"PausePIE"}
```

**Get PIE worlds:**
```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"GetPIEWorlds"}
```

## Engine Built-in APIs

### UEditorLevelLibrary

**CDO Path**: `/Script/EditorScriptingUtilities.Default__EditorLevelLibrary`

| Function | Description |
|----------|-------------|
| `GetPIEWorlds()` | Get worlds from Play-In-Editor sessions (alternative to custom library) |

### UGameplayStatics (on PIE worlds)

**CDO Path**: `/Script/Engine.Default__GameplayStatics`

Once PIE is running, `UGameplayStatics` functions automatically resolve to the PIE world (UCP's WorldContext auto-injection prefers PIE worlds):

| Function | Description |
|----------|-------------|
| `GetPlayerPawn(WorldContextObject, PlayerIndex)` | Get the player pawn in PIE |
| `GetPlayerController(WorldContextObject, PlayerIndex)` | Get the player controller in PIE |
| `GetAllActorsOfClass(WorldContextObject, ActorClass)` | Find actors in the PIE world |

## Common Patterns

### Start PIE, then interact with PIE world

Start a play session, then query actors in the running game (run as separate calls):

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"StartPIE"}
```

After PIE starts, UCP auto-resolves WorldContext to the PIE world:

```json
{"object":"/Script/Engine.Default__GameplayStatics","function":"GetPlayerPawn","params":{"PlayerIndex":0}}
```

### Pause, inspect, resume

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"PausePIE"}
```

Inspect game state while paused, then resume:

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"ResumePIE"}
```

### Check state before starting

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__PIEOperationLibrary","function":"IsInPIE"}
```

If false, start PIE. If true, stop first then restart if needed.

## Key Rules

1. **Only one PIE/SIE session** can be active at a time. Check `IsInPIE` before starting.
2. **StartPIE/StartSimulate are asynchronous** — they queue the request, the session starts on the next editor tick. Subsequent calls to functions that depend on PIE being active should be sent as separate requests.
3. **StopPIE is also asynchronous** — it queues the end request.
4. **WorldContext auto-resolution** — when PIE is active, UCP automatically uses the PIE world for functions requiring `WorldContextObject`.
