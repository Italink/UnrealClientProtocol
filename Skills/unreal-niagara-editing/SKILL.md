---
name: unreal-niagara-editing
description: Edit Niagara systems, emitters, and scripts via UCP. Use when the user asks to create/modify particle effects, manage emitters, configure modules, set module inputs, edit Niagara script graphs, or any Niagara VFX operation.
---

# Niagara Editing

Niagara editing uses three complementary paths: **NiagaraOperationLibrary** for structural operations, **NodeCode** for graph/property reading and writing, and **ObjectOperationLibrary** for detailed UObject property access.

**Prerequisite**: UE editor running with UCP plugin enabled.

## API Overview

### NiagaraOperationLibrary (structural operations)

CDO: `/Script/UnrealClientProtocolEditor.Default__NiagaraOperationLibrary`

UObject path parameters (System, Emitter, Script, etc.) are auto-resolved by UCP — pass the object path as a string and UCP converts it to the UObject pointer.

| Function | Params | Description |
|----------|--------|-------------|
| `AddEmitter` | `System`(path), `SourceEmitter`(path), `Name` | Add emitter to system |
| `RemoveEmitter` | `System`(path), `EmitterName` | Remove emitter from system |
| `ReorderEmitter` | `System`(path), `EmitterName`, `NewIndex` | Reorder emitter |
| `AddUserParameter` | `System`(path), `ParamName`, `TypeName`, `DefaultValue` | Add User parameter |
| `RemoveUserParameter` | `System`(path), `ParamName` | Remove User parameter |
| `AddModule` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleScript`(path), `Index` | Add module to stack |
| `RemoveModule` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid` | Remove module |
| `MoveModule` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `NewIndex` | Reorder module |
| `SetModuleEnabled` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `bEnabled` | Enable/disable module |
| `GetModules` | `SystemOrEmitter`(path), `ScriptUsage` | List modules with name/guid/enabled/scriptPath |
| `SetModuleInputValue` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `Value` | Set input to literal (Rapid Iteration) |
| `SetModuleInputBinding` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `LinkedParamName` | Bind input to parameter |
| `SetModuleInputDynamicInput` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `InputName`, `DynamicInputScript`(path) | Set dynamic input |
| `ResetModuleInput` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid`, `InputName` | Reset to default |
| `GetModuleInputs` | `SystemOrEmitter`(path), `ScriptUsage`, `ModuleNodeGuid` | List inputs with name/type/mode/value |
| `AddRenderer` | `Emitter`(path), `RendererClassName` | Add renderer |
| `RemoveRenderer` | `Emitter`(path), `RendererIndex` | Remove renderer |
| `MoveRenderer` | `Emitter`(path), `RendererIndex`, `NewIndex` | Reorder renderer |
| `AddEventHandler` | `Emitter`(path) | Add event handler |
| `RemoveEventHandler` | `Emitter`(path), `UsageIdString` | Remove event handler |
| `AddSimulationStage` | `Emitter`(path) | Add simulation stage |
| `RemoveSimulationStage` | `Emitter`(path), `StageIdString` | Remove simulation stage |
| `MoveSimulationStage` | `Emitter`(path), `StageIdString`, `NewIndex` | Reorder simulation stage |
| `AddGraphParameter` | `Script`(path), `ParamName`, `TypeName` | Add parameter to script graph |
| `RemoveGraphParameter` | `Script`(path), `ParamName` | Remove parameter |
| `RenameGraphParameter` | `Script`(path), `OldName`, `NewName` | Rename parameter |
| `CreateScratchPadScript` | `SystemOrEmitter`(path), `ScriptName`, `Usage`, `ModuleUsageBitmask` | Create a ScratchPad (Local Module) script |

### NodeCode (graph + properties reading/writing)

CDO: `/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| Function | Params | Description |
|----------|--------|-------------|
| `Outline` | `AssetPath` | List available sections |
| `ReadGraph` | `AssetPath`, `Section` | Read section content as text |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | Write section content back |

## Section Model

### System Asset (UNiagaraSystem)

| Section | Type | Description |
|---------|------|-------------|
| `[SystemProperties]` | Properties | Warmup, bounds, determinism, scalability, LWC, render defaults |
| `[Emitters]` | Properties | Emitter list with names, enabled state, ObjectPath |
| `[UserParameters]` | Properties | User. namespace exposed parameters |
| `[ScratchPadScripts]` | Properties | Local Module scripts with name, Usage, ModuleUsageBitmask, ObjectPath |
| `[SystemSpawn]` | Graph | System spawn stage module chain |
| `[SystemUpdate]` | Graph | System update stage module chain |

### Emitter Object (UNiagaraEmitter, accessed via ObjectPath from [Emitters])

| Section | Type | Description |
|---------|------|-------------|
| `[EmitterProperties]` | Properties | SimTarget, bLocalSpace, bounds, allocation, inheritance |
| `[ParticleAttributes]` | Properties (read-only) | Compiled particle attributes list |
| `[Renderers]` | Properties | Renderer list with class, ObjectPath |
| `[EventHandlers]` | Properties | Event handler configs |
| `[SimulationStages]` | Properties | Simulation stage list |
| `[EmitterSpawn]` | Graph | Emitter spawn stage |
| `[EmitterUpdate]` | Graph | Emitter update stage |
| `[ParticleSpawn]` | Graph | Particle spawn stage |
| `[ParticleUpdate]` | Graph | Particle update stage |
| `[ParticleEvent:EventId]` | Graph | Event handler graph |
| `[SimulationStage:StageId]` | Graph | Custom simulation stage graph |

### Standalone Script (UNiagaraScript)

| Section | Type | Description |
|---------|------|-------------|
| `[Module]` | Graph | Module script graph |
| `[Function]` | Graph | Function script graph |
| `[DynamicInput]` | Graph | Dynamic input script graph |

## ScriptUsage Values

Use these strings for `ScriptUsage` parameter:

`SystemSpawn`, `SystemUpdate`, `EmitterSpawn`, `EmitterUpdate`, `ParticleSpawn`, `ParticleUpdate`, `ParticleEvent`, `SimulationStage`, `Module`, `Function`, `DynamicInput`

## Parameter Namespaces

| Namespace | Scope | Writable In |
|-----------|-------|-------------|
| `System.` | System-level attributes | System stages |
| `Emitter.` | Emitter-level attributes | Emitter stages |
| `Particles.` | Particle attributes | Particle stages |
| `Module.` | Module input parameters | Module internal |
| `User.` | User-exposed parameters | Runtime settable |
| `Engine.` | Engine values (DeltaTime, etc.) | Read-only |
| `Local.Module.` | Module-local temporaries | Module internal |
| `Transient.` | Cross-module temporaries | Current stage |
| `Constants.` | Rapid iteration parameters | Editor-set values |

## Module Input Setting Methods

Modules can have their inputs set in four ways:

1. **Literal Value** (`SetModuleInputValue`) — stored as Rapid Iteration Parameter in `Constants.EmitterName.ModuleName.InputName`, no graph nodes
2. **Binding** (`SetModuleInputBinding`) — ParameterMapGet reads another parameter (e.g. `User.MyParam`, `Emitter.SpawnRate`)
3. **Dynamic Input** (`SetModuleInputDynamicInput`) — a DynamicInput script computes the value
4. **Data Interface** — a NiagaraNodeInput provides a data interface object
5. **Default** — module's built-in default, no override

## Workflow

### Exploring a System

```json
// 1. Get system structure
{"object":"...NodeCodeEditingLibrary","function":"Outline","params":{"AssetPath":"/Game/FX/NS_Fountain"}}

// 2. Read emitter list
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Fountain","Section":"Emitters"}}

// 3. Deep-dive into an emitter (use ObjectPath from step 2)
{"object":"...NodeCodeEditingLibrary","function":"Outline","params":{"AssetPath":"/Game/FX/NS_Fountain.NS_Fountain:Fountain"}}
```

### Modifying Module Stack

```json
// Add a module
{"object":"...NiagaraOperationLibrary","function":"AddModule","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleAssetPath":"/Niagara/Modules/AddVelocity","Index":1}}

// Set module input to a value
{"object":"...NiagaraOperationLibrary","function":"SetModuleInputValue","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleGuid":"...","InputName":"Velocity","Value":"(X=0,Y=0,Z=100)"}}

// Bind module input to User parameter
{"object":"...NiagaraOperationLibrary","function":"SetModuleInputBinding","params":{"EmitterPath":"...Fountain","ScriptUsage":"ParticleSpawn","ModuleGuid":"...","InputName":"Velocity","LinkedParamName":"User.InitialVelocity"}}
```

### Editing a Module Script's Internal Graph

```
ReadGraph("/Game/FX/Modules/MyModule", "Module")
→ Text representation of the node graph

WriteGraph("/Game/FX/Modules/MyModule", "Module", <modified text>)
→ Diff result
```

### Modifying Renderer Properties

```json
// Get renderer list
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"...Fountain","Section":"Renderers"}}

// Read detailed renderer properties via reflection
{"object":"...ObjectOperationLibrary","function":"DescribeObject","params":{"ObjectPath":"...SpriteRenderer_0"}}

// Set renderer property
{"object":"...ObjectOperationLibrary","function":"SetObjectProperty","params":{"ObjectPath":"...SpriteRenderer_0","PropertyName":"Material","JsonValue":"\"/Game/Materials/M_Particle\""}}
```

## Node Types in Graph Sections

| Encoded Name | UClass | Description |
|-------------|--------|-------------|
| `FunctionCall:<ScriptPath>` | UNiagaraNodeFunctionCall | Module/function call (asset module) |
| `FunctionCall:ScratchPad:<Name>` | UNiagaraNodeFunctionCall | ScratchPad (Local Module) reference |
| `Assignment` | UNiagaraNodeAssignment | Inline assignment |
| `CustomHlsl` | UNiagaraNodeCustomHlsl | Custom HLSL code |
| `ParameterMapGet` | UNiagaraNodeParameterMapGet | Read from parameter map |
| `ParameterMapSet` | UNiagaraNodeParameterMapSet | Write to parameter map |
| `NiagaraOp` | UNiagaraNodeOp | Math operation (OpName property) |
| `NiagaraNodeInput` | UNiagaraNodeInput | Input node |
| `NiagaraNodeOutput` | UNiagaraNodeOutput | Output node |

### Working with ScratchPad (Local Module) Scripts

ScratchPad scripts are inline module/function/dynamic-input scripts embedded within a System or Emitter. They appear in `[ScratchPadScripts]` and are referenced in graphs as `FunctionCall:ScratchPad:<Name>`.

```json
// 1. Read ScratchPad list from source system
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Source","Section":"ScratchPadScripts"}}

// 2. Create matching ScratchPad scripts in target system
{"object":"...NiagaraOperationLibrary","function":"CreateScratchPadScript","params":{"SystemOrEmitter":"/Game/FX/NS_Target","ScriptName":"MyModule","Usage":"Module","ModuleUsageBitmask":56}}

// 3. Read source ScratchPad internal graph (use ObjectPath from step 1)
{"object":"...NodeCodeEditingLibrary","function":"ReadGraph","params":{"AssetPath":"/Game/FX/NS_Source.NS_Source:MyModule","Section":"Module"}}

// 4. Write to target ScratchPad (use ObjectPath from step 2)
{"object":"...NodeCodeEditingLibrary","function":"WriteGraph","params":{"AssetPath":"/Game/FX/NS_Target.NS_Target:MyModule","Section":"Module","GraphText":"..."}}

// 5. Now write stage graphs — FunctionCall:ScratchPad:MyModule auto-resolves to target system's script
```

## Important Notes

- **GUID Preservation**: When editing graphs with WriteGraph, preserve existing node `#guid` values to ensure correct diff matching.
- **Emitters are embedded**: Emitters inside a System are copies, not references. Access them via ObjectPath from the `[Emitters]` section.
- **ParticleAttributes are read-only**: They are derived from compiled scripts. To add/remove particle attributes, add/remove modules that write to `Particles.` namespace.
- **Prefer OperationLibrary for module stack changes**: Use AddModule/RemoveModule/SetModuleInput instead of WriteGraph for System/Emitter stage graphs, as these APIs correctly handle Rapid Iteration parameters and Override node management.
