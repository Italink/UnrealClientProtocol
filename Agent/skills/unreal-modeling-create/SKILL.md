---
name: unreal-modeling-create
description: >-
  通过 GeometryScript 建模工作流创建几何体。当用户要求创建图元、生成基于截面的形状，
  或产出 mesh 资产或工作 mesh 用于下游管线时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-modeling
  tags: "geometryscript create mesh generation"
  updated: "2026-03-29"
---

# 建模 — 创建

## 目标

在以下场景使用本 Skill：

- 创建临时工作 mesh 用于后续处理
- 从生成的几何体更新 StaticMesh 资产
- 创建新的几何体结果供上层管线消费

## 决策规则

### 用户需要后续处理的新 mesh

在工作 mesh 阶段停止，除非明确要求资产输出。

### 用户需要可复用的资产结果

先在工作 mesh 中创建几何体，再写入 StaticMesh 资产。

## 标准创建工作流

```text
CreateWorkingMesh
-> GeometryScript 创建/建模函数
-> 可选的写回或资产创建
-> 可选的碰撞最终化
```

## 主要库

- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshPrimitiveFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_MeshModelingFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_SimplePolygonFunctions`
- `/Script/GeometryScriptingCore.Default__GeometryScriptLibrary_PolyPathFunctions`
- `/Script/GeometryScriptingEditor.Default__GeometryScriptLibrary_CreateNewAssetFunctions`

## 配方：创建图元工作 mesh

### 使用场景

需要 Box、Sphere、Cylinder、Capsule、Cone、Disc 或 Torus 作为后续处理的起点。

### 步骤

1. 创建工作 mesh
2. 在工作 mesh 上调用图元追加函数
3. 如需继续进入 meshops / 变形 / UV 步骤
4. 仅在需要资产输出时写回

### 输出

- 默认输出：工作 mesh 路径
- 仅在明确要求时输出资产

## 配方：创建基于截面的几何体

### 使用场景

需要拉伸 / 旋转体 / 扫掠风格的生成。

### 步骤

1. 创建工作 mesh
2. 构造所需的多边形/路径输入
3. 调用相关的 GeometryScript 建模/路径函数
4. 验证几何体
5. 交出工作 mesh 或写回

## 配方：从生成的几何体创建资产输出

### 使用场景

最终结果必须作为真实 mesh 资产存在。

### 步骤

1. 创建并填充工作 mesh
2. 可选预处理法线 / UV / 修复
3. 写回已有资产或创建新资产
4. 如需要，作为单独的最终化步骤生成碰撞

## 常见错误

- 几何体准备完成前过早写入资产
- 跳过工作 mesh 阶段
- 在同一步骤中混合创建和业务层策略决策

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)