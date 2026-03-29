---
name: unreal-actor-editing
description: >-
  通过 UCP 管理 UE 关卡中的 Actor。当用户要求生成、删除、移动、复制、选择或以其他方式操作 Actor 时使用。
metadata:
  version: "1.0.0"
  layer: foundation
  parent: unreal-client-protocol
  tags: "actor spawn delete transform select level"
  updated: "2026-03-29"
---

# Actor 编辑

通过 UCP 的 `call` 命令管理 Unreal Engine 关卡中的 Actor。本 Skill 引导你使用引擎提供的 Actor 子系统和函数库。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## 引擎内置 Actor 函数库

### UEditorActorSubsystem

**CDO 路径**：`/Script/UnrealEd.Default__EditorActorSubsystem`

编辑器中 Actor 操作的主要 API：

| 函数 | 说明 |
|------|------|
| `GetAllLevelActors()` | 获取当前关卡中所有 Actor |
| `GetAllLevelActorsOfClass(ActorClass)` | 按类获取所有 Actor |
| `GetSelectedLevelActors()` | 获取当前选中的 Actor |
| `SetSelectedLevelActors(ActorsToSelect)` | 设置 Actor 选择 |
| `SelectNothing()` | 取消所有选择 |
| `SpawnActorFromClass(ActorClass, Location, Rotation)` | 生成新 Actor |
| `SpawnActorFromObject(ObjectToUse, Location, Rotation)` | 从已有资产生成 Actor |
| `DuplicateActor(ActorToDuplicate, ToWorld, Offset)` | 复制 Actor |
| `DuplicateActors(ActorsToDuplicate, ToWorld, Offset)` | 批量复制 Actor |
| `DestroyActor(ActorToDestroy)` | 删除 Actor |
| `DestroyActors(ActorsToDestroy)` | 批量删除 Actor |
| `SetActorTransform(Actor, WorldTransform)` | 设置 Actor 变换 |
| `ConvertActors(Actors, ActorClass)` | 将 Actor 转换为其他类 |

#### 示例

**获取关卡中所有 Actor：**
```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"GetAllLevelActors"}
```

**获取所有 StaticMesh Actor：**
```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"GetAllLevelActorsOfClass","params":{"ActorClass":"/Script/Engine.StaticMeshActor"}}
```

**生成点光源：**
```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"SpawnActorFromClass","params":{"ActorClass":"/Script/Engine.PointLight","Location":{"X":0,"Y":0,"Z":200}}}
```

**删除 Actor：**
```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"DestroyActor","params":{"ActorToDestroy":"/Game/Maps/Main.Main:PersistentLevel.PointLight_0"}}
```

### UGameplayStatics

**CDO 路径**：`/Script/Engine.Default__GameplayStatics`

运行时/游戏查询：

| 函数 | 说明 |
|------|------|
| `GetAllActorsOfClass(WorldContextObject, ActorClass)` | 按类查找 Actor |
| `GetAllActorsWithTag(WorldContextObject, Tag)` | 按标签查找 Actor |
| `GetAllActorsOfClassWithTag(WorldContextObject, ActorClass, Tag)` | 类 + 标签组合过滤 |
| `GetAllActorsWithInterface(WorldContextObject, Interface)` | 按接口查找 Actor |
| `GetActorOfClass(WorldContextObject, ActorClass)` | 获取指定类的第一个 Actor |
| `FindNearestActor(WorldContextObject, Origin, ActorClass)` | 查找最近的指定类 Actor |
| `GetActorArrayAverageLocation(Actors)` | 获取 Actor 数组的平均位置 |
| `GetActorArrayBounds(Actors, bOnlyCollidingComponents)` | 获取 Actor 数组的组合包围盒 |
| `GetPlayerPawn(WorldContextObject, PlayerIndex)` | 获取玩家 Pawn |
| `GetPlayerController(WorldContextObject, PlayerIndex)` | 获取玩家控制器 |
| `GetPlayerCameraManager(WorldContextObject, PlayerIndex)` | 获取相机管理器 |

### UEditorLevelLibrary

**CDO 路径**：`/Script/EditorScriptingUtilities.Default__EditorLevelLibrary`

UEditorActorSubsystem 未覆盖的关卡编辑功能：

| 函数 | 说明 |
|------|------|
| `JoinStaticMeshActors(ActorsToJoin, JoinOptions)` | 合并静态网格体 |
| `ReplaceSelectedActors(InAssetPath)` | 用指定资产替换选中 Actor |
| `GetPIEWorlds()` | 获取 PIE 会话中的 World |

## 图层管理（ULayersSubsystem）

通过 `FindObjectInstances` 获取实例，`ClassName` 设为 `/Script/UnrealEd.LayersSubsystem`。

| 函数 | 说明 |
|------|------|
| `CreateLayer(LayerName)` | 创建新图层 |
| `DeleteLayer(LayerToDelete)` | 删除图层 |
| `RenameLayer(OriginalLayerName, NewLayerName)` | 重命名图层 |
| `AddActorToLayer(Actor, LayerName)` | 将 Actor 添加到图层 |
| `RemoveActorFromLayer(Actor, LayerName)` | 从图层移除 Actor |
| `AddActorsToLayer(Actors, LayerName)` | 批量添加 Actor 到图层 |
| `GetActorsFromLayer(LayerName)` | 获取图层中所有 Actor |
| `SetLayerVisibility(LayerName, bIsVisible)` | 显示/隐藏图层 |
| `SelectActorsInLayer(LayerName)` | 选中图层中所有 Actor |
| `AddAllLayerNamesTo(OutLayers)` | 获取所有图层名 |

#### 示例

**查找 LayersSubsystem 实例：**
```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"FindObjectInstances","params":{"ClassName":"/Script/UnrealEd.LayersSubsystem"}}
```

**创建图层（使用上面返回的实例路径）：**
```json
{"object":"<layers_subsystem_instance_path>","function":"CreateLayer","params":{"LayerName":"Lighting"}}
```

**将 Actor 添加到图层：**
```json
{"object":"<layers_subsystem_instance_path>","function":"AddActorToLayer","params":{"Actor":"/Game/Maps/Main.Main:PersistentLevel.PointLight_0","LayerName":"Lighting"}}
```

## Actor 分组（UActorGroupingUtils）

CDO 路径 `/Script/UnrealEd.Default__ActorGroupingUtils`，调用 `Get()` 获取单例实例。

| 函数 | 说明 |
|------|------|
| `GroupActors(Actors)` | 将指定 Actor 编组 |
| `UngroupActors(Actors)` | 取消 Actor 编组 |
| `GroupSelected()` | 将当前选中 Actor 编组 |
| `UngroupSelected()` | 取消当前选中 Actor 的编组 |
| `LockSelectedGroups()` | 锁定选中的组 |
| `UnlockSelectedGroups()` | 解锁选中的组 |
| `CanGroupActors()` | 检查当前选择是否可编组 |

#### 示例

**获取分组工具实例：**
```json
{"object":"/Script/UnrealEd.Default__ActorGroupingUtils","function":"Get"}
```

**将选中 Actor 编组（使用 Get 返回的实例）：**
```json
{"object":"<actor_grouping_utils_instance_path>","function":"GroupSelected"}
```

## Actor 文件夹（在 Actor 实例上调用）

直接在 Actor 实例路径上调用：

| 函数 | 说明 |
|------|------|
| `GetFolderPath()` | 获取 Actor 在大纲视图中的文件夹路径 |
| `SetFolderPath(NewFolderPath)` | 设置 Actor 在大纲视图中的文件夹路径 |

#### 示例

**获取 Actor 的文件夹：**
```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.PointLight_0","function":"GetFolderPath"}
```

**将 Actor 移入文件夹：**
```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.PointLight_0","function":"SetFolderPath","params":{"NewFolderPath":"Lighting/Dynamic"}}
```

## Actor 附着（在 Actor 实例上调用）

| 函数 | 说明 |
|------|------|
| `K2_AttachToActor(ParentActor, SocketName, LocationRule, RotationRule, ScaleRule, bWeldSimulatedBodies)` | 将此 Actor 附着到父 Actor |
| `K2_DetachFromActor(LocationRule, RotationRule, ScaleRule)` | 从父 Actor 分离 |

规则为 `EAttachmentRule` 值：`KeepRelative`、`KeepWorld`、`SnapToTarget`。

#### 示例

**附着 Actor：**
```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.Cube_0","function":"K2_AttachToActor","params":{"ParentActor":"/Game/Maps/Main.Main:PersistentLevel.Sphere_0","SocketName":"None","LocationRule":"KeepWorld","RotationRule":"KeepWorld","ScaleRule":"KeepWorld","bWeldSimulatedBodies":false}}
```

**分离 Actor：**
```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.Cube_0","function":"K2_DetachFromActor","params":{"LocationRule":"KeepWorld","RotationRule":"KeepWorld","ScaleRule":"KeepWorld"}}
```

## 关卡管理（ULevelEditorSubsystem + UEditorLevelUtils）

### ULevelEditorSubsystem

通过 `FindObjectInstances` 获取实例，`ClassName` 设为 `/Script/LevelEditor.LevelEditorSubsystem`。

| 函数 | 说明 |
|------|------|
| `NewLevel(AssetPath)` | 创建新关卡 |
| `NewLevelFromTemplate(AssetPath, TemplatePath)` | 从模板创建新关卡 |
| `LoadLevel(AssetPath)` | 加载关卡 |
| `SaveCurrentLevel()` | 保存当前关卡 |
| `SaveAllDirtyLevels()` | 保存所有已修改关卡 |
| `SetCurrentLevelByName(LevelName)` | 切换当前关卡 |
| `GetCurrentLevel()` | 获取当前关卡 |
| `PilotLevelActor(ActorToPilot)` | 在视口中驾驶 Actor |
| `EjectPilotLevelActor()` | 停止驾驶 Actor |

#### 示例

**查找 LevelEditorSubsystem 实例：**
```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"FindObjectInstances","params":{"ClassName":"/Script/LevelEditor.LevelEditorSubsystem"}}
```

**保存当前关卡：**
```json
{"object":"<level_editor_subsystem_instance_path>","function":"SaveCurrentLevel"}
```

### UEditorLevelUtils

**CDO 路径**：`/Script/UnrealEd.Default__EditorLevelUtils`

| 函数 | 说明 |
|------|------|
| `CreateNewStreamingLevel(LevelStreamingClass, NewLevelPath, bMoveSelectedActors)` | 创建新的流式关卡 |
| `MoveActorsToLevel(ActorsToMove, DestLevel, bWarnAboutReferences, bWarnAboutRenaming)` | 将 Actor 移到其他关卡 |
| `K2_AddLevelToWorld(World, LevelPath, LevelStreamingClass)` | 向 World 添加子关卡 |
| `K2_RemoveLevelFromWorld(Level)` | 从 World 移除子关卡 |
| `SetLevelVisibility(Level, bShouldBeVisible, bForceLayersVisible)` | 显示/隐藏关卡 |
| `GetLevels(World)` | 获取 World 中所有关卡 |

## 视口与相机（UUnrealEditorSubsystem）

通过 `FindObjectInstances` 获取实例，`ClassName` 设为 `/Script/UnrealEd.UnrealEditorSubsystem`。

| 函数 | 说明 |
|------|------|
| `GetLevelViewportCameraInfo(OutCameraLocation, OutCameraRotation)` | 获取视口相机位置和旋转 |
| `SetLevelViewportCameraInfo(CameraLocation, CameraRotation)` | 设置视口相机位置和旋转 |
| `GetEditorWorld()` | 获取当前编辑器 World |
| `ScreenToWorld(ScreenPosition)` | 屏幕坐标转世界坐标 |
| `WorldToScreen(WorldPosition)` | 世界坐标转屏幕坐标 |

另可通过 `UEditorUtilityLibrary`：

**CDO 路径**：`/Script/Blutility.Default__EditorUtilityLibrary`

| 函数 | 说明 |
|------|------|
| `GetSelectionBounds(Origin, BoxExtent, SphereRadius)` | 获取当前选择的包围信息 |

#### 示例

**查找 UnrealEditorSubsystem 实例：**
```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"FindObjectInstances","params":{"ClassName":"/Script/UnrealEd.UnrealEditorSubsystem"}}
```

**获取视口相机信息：**
```json
{"object":"<unreal_editor_subsystem_instance_path>","function":"GetLevelViewportCameraInfo"}
```

**设置视口相机位置：**
```json
{"object":"<unreal_editor_subsystem_instance_path>","function":"SetLevelViewportCameraInfo","params":{"CameraLocation":{"X":0,"Y":0,"Z":500},"CameraRotation":{"Pitch":-45,"Yaw":0,"Roll":0}}}
```

**获取选择包围盒：**
```json
{"object":"/Script/Blutility.Default__EditorUtilityLibrary","function":"GetSelectionBounds"}
```

## 常用模式

### 发现和检查 Actor

先获取所有 Actor，再检查特定 Actor（分步调用）：

```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"GetAllLevelActors"}
```

```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"GetObjectProperty","params":{"ObjectPath":"<actor_path>","PropertyName":"RelativeLocation"}}
```

### 移动 Actor

使用 `unreal-object-operation` Skill 设置 Actor 根组件的属性：

```json
{"object":"/Script/UnrealClientProtocol.Default__ObjectOperationLibrary","function":"SetObjectProperty","params":{"ObjectPath":"<actor_root_component_path>","PropertyName":"RelativeLocation","JsonValue":"{\"X\":100,\"Y\":200,\"Z\":0}"}}
```

### 生成、定位并选择

生成 Actor 后获取选择（分步调用）：

```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"SpawnActorFromClass","params":{"ActorClass":"/Script/Engine.PointLight","Location":{"X":0,"Y":0,"Z":300}}}
```

```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"GetSelectedLevelActors"}
```

### 将 Actor 整理到文件夹

```json
{"object":"/Script/UnrealEd.Default__EditorActorSubsystem","function":"GetAllLevelActorsOfClass","params":{"ActorClass":"/Script/Engine.PointLight"}}
```

然后对每个返回的 Actor 设置文件夹路径：

```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.PointLight_0","function":"SetFolderPath","params":{"NewFolderPath":"Lighting"}}
```

### 附着 Actor

```json
{"object":"/Game/Maps/Main.Main:PersistentLevel.ChildActor_0","function":"K2_AttachToActor","params":{"ParentActor":"/Game/Maps/Main.Main:PersistentLevel.ParentActor_0","SocketName":"None","LocationRule":"KeepWorld","RotationRule":"KeepWorld","ScaleRule":"KeepWorld","bWeldSimulatedBodies":false}}
```

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)