---
name: unreal-plugin-localization
description: >-
  翻译 UE 插件本地化 PO 文件。当用户要求本地化、翻译或国际化 Unreal Engine 插件时使用。
  处理 GatherText 采集、PO 导出、AI 翻译、PO 导入和 locres 编译的完整流程。
metadata:
  version: "1.0.0"
  layer: workflow
  tags: "localization translation po i18n"
  updated: "2026-03-29"
---

# 插件本地化

将 Unreal Engine 插件的编辑器 UI 文本翻译为目标语言。工作流使用 Python 辅助脚本完成所有重活（PO 解析、commandlet 调用），Agent 只负责翻译精简 JSON。

## 脚本路径

读取本 SKILL.md 时你已知其绝对路径，将文件名替换为 `scripts/ue_localization.py` 即为脚本路径。**不要搜索或 glob 查找。**

## 工作流

用户提供 `.uplugin` 路径和目标语言。在执行之前，**先询问用户是否启用「语境增强模式」**：

> 是否启用**语境增强模式**？该模式会扫描插件源码和蓝图资产，为每个待翻译条目附加代码上下文（所在文件、行号、周围注释、蓝图变量/函数的 DisplayName 和 Tooltip），帮助产出更准确的翻译。启用后需要更多时间和Token。

用户确认后，按以下三个阶段依次执行。

### 阶段 1 — 采集与导出

运行 `gather` 子命令。脚本自动从 `.uplugin` 向上查找 `.uproject`，通过 `EngineAssociation` + Windows 注册表推断引擎路径，确保 `.uplugin` 中存在 `LocalizationTargets`，然后一次 commandlet 调用完成采集 + 导出 PO，最后将可翻译条目提取为精简 JSON。

```powershell
# 标准模式
python "<脚本路径>" gather --uplugin "<路径>.uplugin" --cultures zh-Hans

# 语境增强模式（加 --context）
python "<脚本路径>" gather --uplugin "<路径>.uplugin" --cultures zh-Hans --context
```

用户选择启用语境增强模式时，加上 `--context` 标志。

脚本通过 stdout 输出 JSON 结果：
- `status`: "ok" 或 "error"
- `pending_files`: `pending_translation.json` 路径列表（每个目标语言一个）
- `context_enhanced`: （仅语境增强模式）`true`
- `context_entries`: （仅语境增强模式）成功提取到上下文的条目总数（C++ + 蓝图）
- `context_blueprint_entries`: （仅语境增强模式且 UCP 可用）从蓝图资产中提取到上下文的条目数量

脚本支持**增量翻译**：PO 文件中已有 `msgstr`（非空翻译）的条目不会出现在 pending 列表中，同时已有翻译会保存到同目录的 `existing_translations.json`。因此 `pending_translation.json` 中的 `total` 可能为 0（表示无需翻译）。

### 阶段 2 — 翻译

先检查 `pending_translation.json` 的 `total` 字段：
- 若 `total` 为 0，**跳过该文件的翻译**，直接进入阶段 3
- 若 `total` > 0，对每个条目执行翻译

对每个需要翻译的 `pending_translation.json`：

1. **读取**该文件，内容为精简条目数组：

```json
{
  "source_lang": "en",
  "target_lang": "zh-Hans",
  "total": 42,
  "entries": [
    {"id": 0, "ctx": "Namespace.Key", "src": "Static Mesh"},
    {"id": 1, "ctx": "Namespace.Key2", "src": "Check resource usage",
     "hint": "[Utils/MeshValidator.cpp:45] | Validates static mesh LOD count"}
  ]
}
```

若启用了语境增强模式，部分条目会包含 `hint` 字段。hint 来源有两种：
- **C++ 源码**：格式为 `[文件:行号] | 周围注释/代码摘要`
- **蓝图资产**（需 UCP 可用）：格式为 `[BP_MyActor.Variable:Health] | ToolTip: Current health points`

翻译时**务必参考 `hint`** 来判断术语在具体场景中的含义，从而选用更恰当的译法。例如同一个 "Instance" 在不同上下文中可能译为 "实例"（对象实例）或 "副本"（场景实例化）。

2. **翻译**每个 `src` 值，遵循下方翻译规范。

3. 在 pending 文件**同目录**下**写入** `translated.json`：

```json
{
  "translations": [
    {"id": 0, "dst": "静态网格体"},
    {"id": 1, "dst": "检查资源使用情况"}
  ]
}
```

pending 文件中的每个 `id` 都必须出现在输出中，不可遗漏。

#### 翻译规范

- 保留 UE 专有名词不翻译：Actor、Blueprint、Widget、Texture2D、StaticMesh、SkeletalMesh、Material、Level、World、HLOD、LOD、Nanite、Lumen、Niagara、PCG 等
- 保留格式占位符原样：`{0}`、`{1}`、`{Arg}`、`%s`、`%d` 等
- 保留文件路径、资产路径、类名原样
- 使用 UE 中文编辑器标准术语（如 "Static Mesh" -> "静态网格体"）
- 简洁 — UI 标签应尽量简短
- 若 `src` 为空，`dst` 也设为空字符串

### 阶段 3 — 注入与编译

运行 `compile` 子命令。脚本自动合并 `existing_translations.json`（已有翻译）和 `translated.json`（新翻译），新翻译优先覆盖已有翻译，然后将合并结果写回 PO 文件的 msgstr 字段，最后一次 commandlet 调用完成导入 + 编译 .locres。

```powershell
python "<脚本路径>" compile --uplugin "<路径>.uplugin" --cultures zh-Hans
```

脚本通过 stdout 输出 JSON 结果，包含 `status` 和详情。

## 完整示例

```powershell
# 阶段 1：采集源文本并导出 PO（语境增强模式加 --context）
python "<脚本路径>" gather --uplugin "C:/Project/Plugins/MyPlugin/MyPlugin.uplugin" --cultures zh-Hans --context

# 阶段 2：Agent 读取 pending_translation.json（含 hint 字段），参考上下文翻译后写入 translated.json

# 阶段 3：注入翻译并编译 locres
python "<脚本路径>" compile --uplugin "C:/Project/Plugins/MyPlugin/MyPlugin.uplugin" --cultures zh-Hans
```

## 补充说明

- 脚本从 `.uplugin` 目录向上遍历自动查找 `.uproject`
- 引擎路径通过 `.uproject` 的 `EngineAssociation` 经 Windows 注册表解析
- 可指定多个目标语言：`--cultures zh-Hans ja ko`
- 源语言默认为 `en`，可通过 `--native en` 覆盖
- INI 配置由脚本内置模板生成，无需外部模板目录
- 标准模式独立运行，不依赖 UCP 或其他 Unreal Skill
- 语境增强模式（`--context`）会额外通过 UCP 读取蓝图资产中的变量/函数 DisplayName 和 Tooltip；UCP 不可用时自动跳过蓝图扫描，仅使用 C++ 上下文

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)