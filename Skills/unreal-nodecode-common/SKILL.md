---
name: unreal-nodecode-common
description: Common rules and conventions for NodeCode-based graph editing (Material, Blueprint, Niagara, Widget). This is a reference skill — domain-specific skills (unreal-material-editing, unreal-blueprint-editing, etc.) should be used for actual operations.
---

# NodeCode Common Rules

NodeCode is the unified text-based representation used by UCP to read/write node graphs across **Materials**, **Blueprints**, **Niagara scripts**, and **Widget trees**. All domain-specific editing skills share the same underlying diff/apply engine. This document describes the common behaviors that apply to all of them.

## API

CDO: `/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| Function | Params | Description |
|----------|--------|-------------|
| `Outline` | `AssetPath` | List available sections for an asset |
| `ReadGraph` | `AssetPath`, `Section` | Read a section's text representation. Empty Section = all. |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | Write a section. Triggers recompile/refresh as appropriate. |

## Node ID Format

- **Existing nodes**: `N_<base62>` — 22-character Base62-encoded GUID. ReadGraph always outputs this form.
- **New nodes**: `N_new<int>` — temporary ID for nodes you are creating (e.g. `N_new0`, `N_new1`). The system assigns a real GUID after WriteGraph.

## Connection Syntax (Graph Sections)

Connections are declared on the **source (output) node**, indented with `>`:

```
  > OutputPin -> N_<target>.InputPin    # named output to named input
  > OutputPin -> [GraphOutput]          # output to graph-level output
  > -> N_<target>.InputPin              # single-output node (omit pin name)
  > -> N_<target>                       # single-output to single-input
```

Domain-specific pin names and graph outputs vary — see each editing skill for details.

## Key Rules

1. **Preserve node IDs** — existing nodes use `N_<base62>` IDs that encode their GUID. Always keep them unchanged when writing back. Use `N_new<int>` only for genuinely new nodes.
2. **ReadGraph before WriteGraph** — always read first to understand the current state.
3. **Undo support** — all WriteGraph operations are wrapped in transactions and support Ctrl+Z.
4. **Incremental diff** — WriteGraph only modifies nodes/connections/properties that actually changed. Unchanged parts are left intact.
5. **Properties are additive, not resetting** — WriteGraph only modifies properties that appear in `{...}`. Omitting a property does **not** reset it to its default value; the existing value is preserved. To change a property back to default, you must write it explicitly (e.g. `{Period:1.000000}` to reset Period). This is critical when correcting previously set non-default values.
6. **Verify all node IDs before writing** — every `N_<id>` referenced in `> ->` connection lines must be defined as a node in the same graph. Referencing an undefined ID silently drops that connection.

## Property Format

Node properties are serialized in `{Key:Value, Key2:Value2}` on the same line as the node declaration. Values use UE reflection import format:

- Scalars: `{R:1.0}`, `{ConstB:0.5}`
- Strings: `{ParameterName:"MyParam"}`
- Structs: `{Constant:(R=1,G=0.5,B=0,A=1)}`
- Booleans: `{R:True}`, `{TwoSided:true}`
- Enums: `{ShadingModel:MSM_Unlit}`

## Error Handling

WriteGraph returns a `diff` object with:
- `nodes_added`, `nodes_removed`, `nodes_modified` — structural changes
- `links_added`, `links_removed` — connection changes
- `compile_errors` — compilation errors (if applicable)

Common errors:
- `"Property not found: ..."` — typo in property name
- `"Failed to import ..."` — invalid property value format
- `"Input 'X' not found on N_... (ClassName). Available: [...]"` — wrong pin name
