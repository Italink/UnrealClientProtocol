---
name: unreal-widget-editing
description: >-
  通过文本化读写（ReadGraph/WriteGraph）编辑 UMG Widget 蓝图 UI 布局。当用户要求添加、删除或重排 UI 控件，
  修改控件属性或修改 Widget 蓝图的视觉层级时使用。
metadata:
  version: "1.0.0"
  layer: domain
  parent: unreal-nodecode-common
  tags: "nodecode widget umg ui layout"
  updated: "2026-03-29"
---

# Widget 编辑

通过文本化树表示编辑 UMG Widget 蓝图 UI 布局。Widget 树（视觉层级）使用 `[WidgetTree]` 区段。编辑 Widget 蓝图的**事件图**和**函数**时，使用 `unreal-blueprint-editing` Skill — Widget 蓝图就是蓝图，EventGraph/Function/Variables 区段的工作方式完全相同。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## API

CDO：`/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary`

| 函数 | 参数 | 说明 |
|------|------|------|
| `Outline` | `AssetPath` | 返回所有区段：`[WidgetTree]`、`[Variables]`、`[EventGraph]`、`[Function:Name]` 等。 |
| `ReadGraph` | `AssetPath`, `Section` | 返回文本。UI 布局使用 `"WidgetTree"`。 |
| `WriteGraph` | `AssetPath`, `Section`, `GraphText` | 覆写 Widget 树。 |

## Widget 蓝图的区段类型

Widget 蓝图的 Outline 返回 Widget 树和图区段：

| 区段 | 处理器 | 说明 |
|------|--------|------|
| `WidgetTree` | WidgetTreeSectionHandler | UI 控件层级 |
| `Variables` | BlueprintSectionHandler | 蓝图变量 |
| `EventGraph` | BlueprintSectionHandler | 事件图逻辑 |
| `Function:<Name>` | BlueprintSectionHandler | 蓝图函数 |

## WidgetTree 文本格式

Widget 树使用**基于缩进**的文本格式，深度表示父子关系：

```
[WidgetTree]
CanvasPanel_0: {"Type":"CanvasPanel"}
  Image_BG: {"Type":"Image", "Slot":{"Anchors":"(Minimum=(X=0,Y=0),Maximum=(X=1,Y=1))", "Offsets":"(Left=0,Top=0,Right=0,Bottom=0)"}}
  VerticalBox_Main: {"Type":"VerticalBox", "Slot":{"Anchors":"(Minimum=(X=0.5,Y=0.5),Maximum=(X=0.5,Y=0.5))"}}
    TextBlock_Title: {"Type":"TextBlock", "Text":"Hello World", "Slot":{"HorizontalAlignment":"HAlign_Center"}}
    Button_Submit: {"Type":"Button", "Slot":{"Padding":"(Left=0,Top=10,Right=0,Bottom=0)"}, "bIsVariable":true}
      TextBlock_BtnLabel: {"Type":"TextBlock", "Text":"Submit"}
```

### 格式规则

- **缩进 = 层级**：每级 2 个空格。根控件无缩进。
- **控件名是键**：`TextBlock_Title`、`Button_Submit` 等。这些是控件的 `GetName()`。
- **`Type`**：控件类名（如 `CanvasPanel`、`TextBlock`、`Button`、`Image`）。通过反射解析。
- **`Slot`**：来自父面板插槽类型的布局属性，作为嵌套 JSON 对象。
- **`bIsVariable`**：为 `true` 时控件可从蓝图图中访问（如绑定事件或运行时设置属性）。仅在 `true` 时序列化。
- **其他属性**：非默认控件属性，通过 UE 反射序列化。

### 不同面板类型的 Slot 属性

不同面板类型使用不同的插槽类型和布局属性：

**CanvasPanelSlot**（父为 CanvasPanel）：
- `LayoutData.Anchors`：`(Minimum=(X=0,Y=0),Maximum=(X=1,Y=1))` — 锚点预设
- `LayoutData.Offsets`：`(Left=0,Top=0,Right=0,Bottom=0)` — 位置/尺寸偏移
- `LayoutData.Alignment`：`(X=0.5,Y=0.5)` — 枢轴对齐
- `bAutoSize`：`true/false`
- `ZOrder`：整数

**VerticalBoxSlot / HorizontalBoxSlot**（父为 VerticalBox/HorizontalBox）：
- `Padding`：`(Left=0,Top=10,Right=0,Bottom=0)`
- `Size`：`(SizeRule=Fill,Value=1.0)` 或 `(SizeRule=Auto,Value=1.0)`
- `HorizontalAlignment`：`HAlign_Fill`、`HAlign_Left`、`HAlign_Center`、`HAlign_Right`
- `VerticalAlignment`：`VAlign_Fill`、`VAlign_Top`、`VAlign_Center`、`VAlign_Bottom`

**OverlaySlot**（父为 Overlay）：
- `Padding`：`(Left=0,Top=0,Right=0,Bottom=0)`
- `HorizontalAlignment`：同上
- `VerticalAlignment`：同上

**GridSlot**（父为 GridPanel）：
- `Row`、`Column`：整数
- `RowSpan`、`ColumnSpan`：整数

### 常用控件类型

| 类型 | 说明 | 关键属性 |
|------|------|----------|
| `CanvasPanel` | 自由定位容器 | （无典型属性） |
| `VerticalBox` | 垂直排列子控件 | （无典型属性） |
| `HorizontalBox` | 水平排列子控件 | （无典型属性） |
| `Overlay` | 子控件层叠 | （无典型属性） |
| `GridPanel` | 网格布局 | （无典型属性） |
| `ScrollBox` | 可滚动容器 | `Orientation` |
| `SizeBox` | 约束子控件尺寸 | `WidthOverride`、`HeightOverride` |
| `Border` | 带背景的单子控件 | `Background` |
| `TextBlock` | 显示文本 | `Text`、`Font`、`ColorAndOpacity` |
| `RichTextBlock` | 富文本显示 | `Text` |
| `Image` | 显示图片/纹理 | `Brush` |
| `Button` | 可点击按钮 | `WidgetStyle` |
| `CheckBox` | 复选框 | `IsChecked` |
| `Slider` | 值滑块 | `Value`、`MinValue`、`MaxValue` |
| `ProgressBar` | 进度指示器 | `Percent` |
| `EditableText` | 单行文本输入 | `Text`、`HintText` |
| `EditableTextBox` | 带边框的单行文本输入 | `Text`、`HintText` |
| `MultiLineEditableText` | 多行文本输入 | `Text` |
| `ComboBoxString` | 下拉选择 | `DefaultOptions` |
| `SpinBox` | 数值输入 | `Value`、`MinValue`、`MaxValue` |
| `Spacer` | 空白间距控件 | `Size` |
| `WidgetSwitcher` | 一次显示一个子控件 | `ActiveWidgetIndex` |

## 工作流

1. **Outline** — 查看可用区段（WidgetTree + 图区段）
2. **ReadGraph("WidgetTree")** — 获取 UI 层级
3. **理解树结构** — 缩进表示父子关系
4. **修改** — 添加/删除/重排控件，修改属性
5. **WriteGraph("WidgetTree", text)** — 应用变更

逻辑变更（事件绑定、运行时属性更新）：
1. **ReadGraph("EventGraph")** — 使用 `unreal-blueprint-editing` Skill
2. 通过名称引用控件（`bIsVariable:true` 的控件可在图中访问）

## 关键规则

1. **缩进很重要**：每级 2 个空格。错误的缩进会创建错误的父子关系。
2. **控件名在树中必须唯一**。
3. **根控件**（深度 0）必须是面板控件（CanvasPanel、VerticalBox 等）。
4. **只有面板控件**可以有子控件。不要在 TextBlock 等非面板控件下缩进子控件。
5. **需要从蓝图图中引用的控件设置 `bIsVariable:true`**。
6. **先 ReadGraph 再 WriteGraph** — 始终先读取以了解当前结构。
7. **Slot 属性**取决于父面板类型。VerticalBox 下的控件使用 VerticalBoxSlot 属性，而非 CanvasPanelSlot。
8. 所有操作支持 **Undo**（Ctrl+Z）。

## 错误处理

- 未知控件类：`"Unknown widget class: ..."`。
- 创建控件失败：`"Failed to create widget: ..."`。
- 检查响应中的 `diff` 对象了解 `nodes_added`、`nodes_removed`。

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)