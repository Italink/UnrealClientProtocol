# Skill 自迭代架构规范

本文档定义 UCP Skill 体系的分层结构、元数据规范和自迭代机制。所有 Skill 的创建和演化必须遵守这些规范。

## 设计原则

1. **只用 Cursor 原生机制**：SKILL.md `metadata` 字段、`.cursor/rules/*.mdc`、`.cursor/hooks.json` —— 不发明私有格式
2. **向前兼容**：agentskills.io 规范正在推进 `version`/`tags` 等顶层字段标准化，当前扩展字段放在 `metadata` map 中，标准落地时可一键提升为顶层字段
3. **渐进式加载**：遵循 SKILL.md 的 Progressive Disclosure 模型（metadata ~100 tokens → body <5000 tokens → references/ 按需），分层信息不增加 Discovery 阶段的 token 开销
4. **数据驱动迭代**：通过 Hooks 收集真实使用数据，而非靠猜测

## 目录结构

```
Plugins/UnrealClientProtocol/
├── Agent/                          # 统一分发目录
│   ├── skills/                     # SKILL.md 文件（source of truth）
│   │   ├── unreal-client-protocol/
│   │   │   ├── SKILL.md
│   │   │   └── scripts/UCP.py
│   │   ├── unreal-material-editing/
│   │   │   └── SKILL.md
│   │   └── ...
│   ├── rules/                      # Cursor Rules（随插件分发）
│   │   ├── skill-taxonomy.mdc
│   │   └── skill-evolution.mdc
│   └── scripts/                    # 工具脚本
│       ├── sync-agent.ps1
│       └── skill-report.py
├── Source/
├── Docs/
└── README.md
```

安装后同步到：
- `Agent/skills/*` → `.cursor/skills/`
- `Agent/rules/*` → `.cursor/rules/`

## SKILL.md 元数据规范

### Front-matter 格式

```yaml
---
name: unreal-material-editing
description: >-
  通过文本化读写（ReadGraph/WriteGraph）编辑 UE 材质节点图和属性。
  当用户要求添加、删除或重连材质表达式节点时使用。
metadata:
  version: "1.2.0"
  layer: domain
  parent: unreal-nodecode-common
  tags: "nodecode material graph-editing expression"
  updated: "2026-03-29"
---
```

### metadata 字段定义

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `version` | string | 是 | 语义化版本（major.minor.patch） |
| `layer` | string | 是 | 层级枚举，见下表 |
| `parent` | string | 否 | 依赖的上层 Skill id |
| `tags` | string | 是 | 空格分隔的标签，辅助语义匹配 |
| `updated` | string | 是 | 最后更新日期（YYYY-MM-DD） |

### layer 枚举

| 值 | 含义 | 示例 |
|----|------|------|
| `transport` | 协议传输层 | unreal-client-protocol |
| `foundation` | 基础操作层 | unreal-object-operation, unreal-actor-editing |
| `convention` | 公共约定层 | unreal-nodecode-common |
| `domain` | 领域编辑层 | unreal-material-editing, unreal-modeling |
| `composite` | 复合流程层 | unreal-large-blueprint-analysis |
| `workflow` | 独立工作流 | unreal-plugin-localization |

### parent 约束

- `transport` 层 Skill 无 parent
- `foundation` 和 `convention` 层的 parent 为 `unreal-client-protocol`
- `domain` 层图编辑 Skill 的 parent 为 `unreal-nodecode-common`
- `domain` 层建模子 Skill 的 parent 为 `unreal-modeling`
- `composite` 层 Skill 的 parent 为其主要依赖的 Skill
- `workflow` 层 Skill 无 parent（独立工作流）

### 版本号规则

- **patch**（x.x.+1）：修复错误、更新示例、措辞改进
- **minor**（x.+1.0）：新增能力、新增 API 覆盖
- **major**（+1.0.0）：破坏性变更（格式变化、移除功能）

## SKILL.md Body 规范

### 必须包含的 Section

每个 SKILL.md body 末尾必须包含：

```markdown
## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)
```

- `Changelog`：按版本倒序排列，最新在前
- `Known Issues`：记录已知但未解决的问题，附日期。这是最简单的"跨会话记忆"机制

### 不应包含的内容

以下信息已由 `metadata` 和 `skill-taxonomy.mdc` 覆盖，不应在 SKILL.md body 中重复：

- 层级自述（"这是传输层"、"这是基础 Skill"）
- 前置条件中的 Skill 依赖（"先阅读 unreal-modeling"）
- 跨 Skill 路由索引（"具体操作见领域 Skill"）

以下内容应保留在 SKILL.md body 中：

- 环境前置条件（"UE 编辑器运行中，UCP 插件已启用"）
- 技术性 API 路由（"材质实例参数编辑使用 SetObjectProperty 而非 NodeCode"）
- 领域专属的操作指南和示例

## 分层路由机制

### skill-taxonomy.mdc

`Agent/rules/skill-taxonomy.mdc` 是一个 `alwaysApply: true` 的 Cursor Rule，约 200 tokens，描述完整的 Skill 层次结构和选择策略。它在每次对话中自动注入，指导 Agent 的 Skill 选择行为。

当 Cursor 未来支持 Skill 的原生分类路由时（基于 `metadata.layer` 和 `metadata.tags`），这个 Rule 可以直接删除，因为分层信息已经在每个 SKILL.md 的 metadata 中了。

### 选择策略

1. 根据用户意图确定所需的 LAYER
2. 在 domain 层内确定 SUB-DOMAIN（graph-editing vs modeling）
3. 不确定 UCP 调用方式时，先加载 transport 层知识
4. 图编辑任务：先加载 convention 层（unreal-nodecode-common）再加载 domain 层
5. 建模任务：先加载 unreal-modeling 再加载子 Skill

## 使用追踪机制

### Hooks 架构

通过 Cursor 的 Hooks 系统收集 Skill 使用数据：

```
.cursor/
├── hooks.json                      # Hook 配置
└── hooks/
    ├── track-skill-usage.py        # postToolUse(Read) → 检测 SKILL.md 读取
    ├── session-summary.py          # stop → 汇总会话 Skill 使用
    └── state/
        ├── skill-usage.jsonl       # 逐次 Skill 加载记录
        └── session-log.jsonl       # 会话级汇总
```

### 数据格式

**skill-usage.jsonl**（每次 SKILL.md 被读取时追加）：
```json
{"timestamp":"2026-03-29T10:30:00Z","skill":"unreal-material-editing","conversation_id":"abc","model":"claude-4"}
```

**session-log.jsonl**（每次会话结束时追加）：
```json
{"conversation_id":"abc","status":"completed","skills_used":["unreal-material-editing","unreal-nodecode-common"],"timestamp":"2026-03-29T10:45:00Z"}
```

### 分析报告

`Agent/scripts/skill-report.py` 读取 session-log.jsonl 生成报告：
- 各 Skill 使用频率排名
- 失败会话中涉及的 Skill（自迭代候选）
- Skill 共现关系
- 从未使用的 Skill

## 自迭代协议

### skill-evolution.mdc

`Agent/rules/skill-evolution.mdc` 是一个 `alwaysApply: false` 的 Cursor Rule，仅在用户要求改进 Skill 时智能激活。它定义了完整的自迭代流程：

```
分析 → 提案 → 审核 → 执行 → 验证
```

### 触发条件

- 用户明确说"这个 Skill 不好用"或"帮我改进 XX Skill"
- Agent 在同一类任务上连续失败
- skill-report.py 报告中出现高失败率 Skill

### 流程

1. **分析**：读取 SKILL.md（含 Changelog 和 Known Issues）+ session-log.jsonl
2. **提案**：生成具体的修改 diff + 理由说明
3. **审核**：用户确认后执行
4. **执行**：编辑 SKILL.md、递增 version、更新 Changelog
5. **同步**：运行 sync-agent.ps1 传播到 .cursor/

### 新 Skill 发现

当分析发现能力缺口时：
1. 判断是扩展现有 Skill 还是创建新 Skill
2. 新 Skill 必须确定正确的 layer 和 parent
3. 生成完整的 SKILL.md（含 metadata、Changelog、Known Issues）
4. 更新 skill-taxonomy.mdc

## 与业界前沿的对应关系

| 前沿方案 | 核心思想 | UCP 落地方式 |
|---------|---------|-------------|
| EvoSkill | 失败驱动 + Skill Folder | skill-evolution.mdc + Hooks 失败追踪 + SKILL.md 文件夹结构 |
| AgentSkillOS | 能力树 + DAG 编排 | skill-taxonomy.mdc（软路由）+ metadata.layer/parent |
| AutoSkill | 对话轨迹提炼 | session-log.jsonl + skill-report.py + Known Issues |
| SKILL.md 标准化 | metadata 扩展字段 | 全部在 metadata map 中，标准落地时一键提升 |
