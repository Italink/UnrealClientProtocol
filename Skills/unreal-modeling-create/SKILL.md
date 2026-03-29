---
name: unreal-modeling-create
description: Create geometry through the GeometryScript modeling workflow. Use when the user asks to create primitives, generate profile-based shapes, or produce mesh assets or working meshes for downstream pipelines.
---

# Modeling — Create

Prerequisite: read `unreal-modeling-core` first.

## Goal

Use this skill when the output should be one of the following:

- a transient working mesh for further processing
- a static mesh asset updated from generated geometry
- a new geometry result that will be consumed by a higher-level pipeline

## Required decision rules

### If the user wants a new mesh to be processed later

Stop at the working mesh stage unless asset output is explicitly required.

### If the user wants a reusable asset result

Create geometry in a working mesh first, then write it to a static mesh asset.

## Standard create workflow

```text
CreateWorkingMesh
-> GeometryScript create/modeling functions
-> optional writeback or asset creation
-> optional collision finalization
```

## Primary libraries

- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshModelingFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_SimplePolygonFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_PolyPathFunctions`
- `/Script/GeometryScriptingEditor.Default__GeometryScriptLibrary_CreateNewAssetFunctions`

## Recipe: create a primitive working mesh

### Use when

You need a box, sphere, cylinder, capsule, cone, disc, or torus as the starting point for later processing.

### Steps

1. create a working mesh
2. call a primitive append function on the working mesh
3. if needed, continue into meshops / deform / UV steps
4. only write back if asset output is required

### Output

- default output: working mesh path
- asset output only if explicitly requested

## Recipe: create profile-based geometry

### Use when

You need extrude / revolve / sweep style generation.

### Steps

1. create a working mesh
2. construct the required polygon/path inputs
3. call the relevant GeometryScript modeling/path functions
4. validate geometry
5. hand off the working mesh or write back

## Recipe: create asset output from generated geometry

### Use when

The end result must exist as a real mesh asset.

### Steps

1. create and populate a working mesh
2. optionally preprocess normals / UV / repair
3. write back to an existing asset or create a new asset
4. if required, generate collision as a separate finalization step

## Common mistakes to avoid

- writing to assets too early before geometry preparation is complete
- skipping the working mesh stage
- combining creation and business-layer policy decisions in the same step
