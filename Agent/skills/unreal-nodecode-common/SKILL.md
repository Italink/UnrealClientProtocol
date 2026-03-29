---
name: unreal-nodecode-common
description: >-
  NodeCode 节点图编辑的通用规则和约定（材质、蓝图、Niagara、Widget）。
metadata:
  version: "1.0.0"
  layer: convention
  parent: unreal-client-protocol
  tags: "nodecode graph-editing common rules"
  updated: "2026-03-29"
---

# NodeCode 通用规则

NodeCode 是 UCP 用于跨**材质**、**蓝图**、**Niagara 脚本**和 **Widget 树**读写节点图的统一文本化表示。所有领域编辑 Skill 共享同一套底层 diff/apply 引擎。本文档描述适用于所有图类型的通用行为。

## API

CDO：`/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| 函数 | 参数 | 说明 |
|------|------|------|
| `Outline` | `AssetPath` | 列出资产的可用区段 |
| `ReadGraph` | `AssetPath`, `Section` | 读取区段的文本表示。Section 为空 = 全部。 |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | 写入区段。按需触发重编译/刷新。 |

## 节点 ID 格式

- **已有节点**：`N_<base62>` — 22 字符 Base62 编码的 GUID。ReadGraph 始终输出此形式。
- **新节点**：`N_new<int>` — 你创建的节点的临时 ID（如 `N_new0`、`N_new1`）。WriteGraph 后系统分配真实 GUID。

## 连接语法（图区段）

连接在**源（输出）节点**上声明，缩进 `>`：

```
  > OutputPin -> N_<target>.InputPin    # 命名输出到命名输入
  > OutputPin -> [GraphOutput]          # 输出到图级别输出
  > -> N_<target>.InputPin              # 单输出节点（省略引脚名）
  > -> N_<target>                       # 单输出到单输入
```

领域特定的引脚名和图输出各不相同 — 详见各编辑 Skill。

## 关键规则

1. **保持节点 ID 不变** — 已有节点使用 `N_<base62>` ID 编码其 GUID。写回时始终保持不变。仅对真正的新节点使用 `N_new<int>`。
2. **先 ReadGraph 再 WriteGraph** — 始终先读取以了解当前状态。
3. **Undo 支持** — 所有 WriteGraph 操作都封装在事务中，支持 Ctrl+Z。
4. **增量 diff** — WriteGraph 仅修改实际变化的节点/连接/属性。未变更部分保持不动。
5. **属性是累加的，不会重置** — WriteGraph 仅修改 `{...}` 中出现的属性。省略属性**不会**将其重置为默认值；已有值被保留。要将属性改回默认值，必须显式写入（如 `{Period:1.000000}` 重置 Period）。这在纠正之前设置的非默认值时至关重要。
6. **写入前验证所有节点 ID** — `> ->` 连接行中引用的每个 `N_<id>` 都必须在同一图中定义为节点。引用未定义的 ID 会静默丢弃该连接。

## 属性格式

节点属性序列化在节点声明同一行的 `{Key:Value, Key2:Value2}` 中。值使用 UE 反射导入格式：

- 标量：`{R:1.0}`、`{ConstB:0.5}`
- 字符串：`{ParameterName:"MyParam"}`
- 结构体：`{Constant:(R=1,G=0.5,B=0,A=1)}`
- 布尔：`{R:True}`、`{TwoSided:true}`
- 枚举：`{ShadingModel:MSM_Unlit}`

## 错误处理

WriteGraph 返回 `diff` 对象，包含：
- `nodes_added`、`nodes_removed`、`nodes_modified` — 结构变更
- `links_added`、`links_removed` — 连接变更
- `compile_errors` — 编译错误（如适用）

常见错误：
- `"Property not found: ..."` — 属性名拼写错误
- `"Failed to import ..."` — 属性值格式无效
- `"Input 'X' not found on N_... (ClassName). Available: [...]"` — 引脚名错误

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)