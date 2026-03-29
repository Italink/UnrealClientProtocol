---
name: unreal-asset-operation
description: >-
  通过 UCP 查询和管理 UE 资产。当用户要求搜索资产、查看依赖/引用、资产增删改查、获取选中/当前资产、
  打开/关闭资产编辑器或任何编辑器资产操作时使用。
metadata:
  version: "1.0.0"
  layer: foundation
  parent: unreal-client-protocol
  tags: "asset registry search crud editor"
  updated: "2026-03-29"
---

# 资产操作

通过 UCP 的 `call` 命令操作 Unreal Engine 资产。核心方式是获取 `IAssetRegistry` 实例并直接调用其丰富的 API，结合引擎资产库完成 CRUD 操作。

**前置条件**：UE 编辑器运行中，UCP 插件已启用。

## 自定义函数库

### UAssetEditorOperationLibrary（编辑器）

**CDO 路径**：`/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary`

| 函数 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `GetAssetRegistry` | （无） | `IAssetRegistry` 对象 | 返回 AssetRegistry 实例，可直接通过 UCP `call` 调用其函数。 |
| `ForceDeleteAssets` | `AssetPaths`（字符串数组） | `int32` — 删除数量 | 强制删除资产（忽略引用）。封装 `ObjectTools::ForceDeleteObjects`。 |
| `FixupReferencers` | `AssetPaths`（字符串数组） | `bool` | 清理重命名/合并后残留的重定向器。封装 `IAssetTools::FixupReferencers`。 |

#### 获取 AssetRegistry

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"GetAssetRegistry"}
```

返回的对象路径可作为后续 IAssetRegistry 函数调用的 `object` 参数。

也可直接使用引擎辅助函数：

```json
{"object":"/Script/AssetRegistry.Default__AssetRegistryHelpers","function":"GetAssetRegistry"}
```

#### 强制删除资产

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"ForceDeleteAssets","params":{"AssetPaths":["/Game/OldAssets/M_Unused","/Game/OldAssets/T_Unused"]}}
```

#### 修复重定向器

重命名或合并资产后可能残留重定向器，用 `FixupReferencers` 清理：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"FixupReferencers","params":{"AssetPaths":["/Game/Materials/M_OldName"]}}
```

## 引擎内置资产库

### IAssetRegistry（通过 GetAssetRegistry 获取）

获取 AssetRegistry 实例后，可调用以下 BlueprintCallable 函数：

#### 资产查询

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetAssetsByPackageName` | `PackageName`, `OutAssetData`, `bIncludeOnlyOnDiskAssets` | 获取指定包中的资产 |
| `GetAssetsByPath` | `PackagePath`, `OutAssetData`, `bRecursive` | 获取路径下的资产 |
| `GetAssetsByPaths` | `PackagePaths`, `OutAssetData`, `bRecursive` | 获取多个路径下的资产 |
| `GetAssetsByClass` | `ClassPathName`, `OutAssetData`, `bSearchSubClasses` | 按类获取所有资产 |
| `GetAssets` | `Filter`, `OutAssetData` | 使用 `FARFilter` 查询（强大的过滤搜索） |
| `GetAllAssets` | `OutAssetData`, `bIncludeOnlyOnDiskAssets` | 获取所有已注册资产 |
| `GetAssetByObjectPath` | `ObjectPath` | 按路径获取单个资产数据 |
| `HasAssets` | `PackagePath`, `bRecursive` | 检查路径下是否存在资产 |
| `GetInMemoryAssets` | `Filter`, `OutAssetData` | 仅查询当前已加载的资产 |

#### 依赖与引用查询

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetDependencies` | `PackageName`, `DependencyOptions`, `OutDependencies` | 获取资产依赖的包 |
| `GetReferencers` | `PackageName`, `ReferenceOptions`, `OutReferencers` | 获取引用此资产的包 |

#### 类层级

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetAncestorClassNames` | `ClassPathName`, `OutAncestorClassNames` | 获取父类 |
| `GetDerivedClassNames` | `ClassNames`, `DerivedClassNames` | 获取子类 |

#### 路径与扫描

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `GetAllCachedPaths` | `OutPathList` | 获取所有已知内容路径 |
| `GetSubPaths` | `InBasePath`, `OutPathList`, `bInRecurse` | 获取基础路径下的子路径 |
| `ScanPathsSynchronous` | `InPaths`, `bForceRescan` | 强制扫描指定路径 |
| `ScanFilesSynchronous` | `InFilePaths`, `bForceRescan` | 强制扫描指定文件 |
| `SearchAllAssets` | `bSynchronousSearch` | 触发全量资产扫描 |
| `ScanModifiedAssetFiles` | `InFilePaths` | 扫描已修改的文件 |
| `IsLoadingAssets` | （无） | 检查扫描是否进行中 |
| `WaitForCompletion` | （无） | 阻塞直到扫描完成 |

#### 过滤与排序

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `RunAssetsThroughFilter` | `AssetDataList`, `Filter` | 原地过滤资产列表（保留匹配） |
| `UseFilterToExcludeAssets` | `AssetDataList`, `Filter` | 原地过滤资产列表（移除匹配） |

#### UAssetRegistryHelpers（静态工具）

**CDO 路径**：`/Script/AssetRegistry.Default__AssetRegistryHelpers`

| 函数 | 说明 |
|------|------|
| `GetAssetRegistry` | 获取 IAssetRegistry 实例 |
| `GetDerivedClassAssetData` | 获取派生类的资产数据 |
| `SortByAssetName` | 按名称排序 FAssetData 数组 |
| `SortByPredicate` | 按自定义谓词排序 FAssetData 数组 |

### IAssetTools（通过 UAssetToolsHelpers::GetAssetTools()）

获取实例：

```json
{"object":"/Script/AssetTools.Default__AssetToolsHelpers","function":"GetAssetTools"}
```

然后在返回的实例上调用函数：

#### 创建

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `CreateAsset` | `AssetName`, `PackagePath`, `AssetClass`, `Factory` | 创建新资产（简单类型 Factory 可为 null） |
| `CreateUniqueAssetName` | `InBasePackageName`, `InSuffix` | 生成唯一名称避免冲突 |
| `DuplicateAsset` | `AssetName`, `PackagePath`, `OriginalObject` | 复制已有资产 |

##### 示例：创建新材质

```json
{"object":"<asset_tools_instance>","function":"CreateAsset","params":{"AssetName":"M_NewMaterial","PackagePath":"/Game/Materials","AssetClass":"/Script/Engine.Material","Factory":null}}
```

#### 导入与导出

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `ImportAssetTasks` | `ImportTasks`（UAssetImportTask 数组） | 导入外部文件为资产 |
| `ImportAssetsAutomated` | `ImportData`（UAutomatedAssetImportData） | 自动化批量导入 |
| `ExportAssets` | `AssetsToExport`, `ExportPath` | 导出资产为文件 |

#### 重命名与移动

| 函数 | 关键参数 | 说明 |
|------|----------|------|
| `RenameAssets` | `AssetsAndNames`（FAssetRenameData 数组） | 批量重命名/移动资产 |
| `FindSoftReferencesToObject` | `TargetObject` | 查找对象的软引用 |
| `RenameReferencingSoftObjectPaths` | `PackagesToCheck`, `AssetRedirectorMap` | 重命名后更新软引用 |
| `MigratePackages` | `PackageNamesToMigrate`, `DestinationPath` | 迁移包到其他项目 |

##### 示例：批量重命名资产

`RenameAssets` 接受 `FAssetRenameData` 结构体数组。每个结构体包含 `Asset`（要重命名的对象）、`NewPackagePath`（目标目录）和 `NewName`（新名称）：

```json
{"object":"<asset_tools_instance>","function":"RenameAssets","params":{"AssetsAndNames":[{"Asset":"/Game/Materials/M_OldName.M_OldName","NewPackagePath":"/Game/Materials","NewName":"M_NewName"}]}}
```

##### 示例：查找软引用

```json
{"object":"<asset_tools_instance>","function":"FindSoftReferencesToObject","params":{"TargetObject":"/Game/Materials/M_Example.M_Example"}}
```

### UEditorAssetLibrary

**CDO 路径**：`/Script/EditorScriptingUtilities.Default__EditorAssetLibrary`

#### 查询与检查

| 函数 | 说明 |
|------|------|
| `DoesAssetExist(AssetPath)` | 检查资产是否存在 |
| `DoAssetsExist(AssetPaths)` | 批量检查存在性 |
| `ListAssets(DirectoryPath, bRecursive, bIncludeFolder)` | 列出目录中的资产 |
| `ListAssetByTagValue(AssetTagName, TagValue)` | 按标签值列出资产 |
| `LoadAsset(AssetPath)` | 将资产加载到内存 |
| `LoadBlueprintClass(AssetPath)` | 加载蓝图并返回其生成类 |
| `GetTagValues(AssetPath, AssetTagName)` | 获取资产的标签值 |
| `FindPackageReferencersForAsset(AssetPath, bLoadAssetsToConfirm)` | 查找引用此资产的所有包 |

##### 示例：查找资产引用

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"FindPackageReferencersForAsset","params":{"AssetPath":"/Game/Materials/M_Example","bLoadAssetsToConfirm":false}}
```

##### 示例：按标签列出资产

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"ListAssetByTagValue","params":{"AssetTagName":"MyCustomTag","TagValue":"SomeValue"}}
```

##### 示例：加载蓝图类

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"LoadBlueprintClass","params":{"AssetPath":"/Game/Blueprints/BP_MyActor"}}
```

#### 创建与修改

| 函数 | 说明 |
|------|------|
| `DuplicateAsset(SourceAssetPath, DestinationAssetPath)` | 按路径复制资产 |
| `DuplicateDirectory(SourceDirectoryPath, DestinationDirectoryPath)` | 复制整个资产目录 |
| `RenameAsset(SourceAssetPath, DestinationAssetPath)` | 按路径重命名/移动资产 |
| `RenameDirectory(SourceDirectoryPath, DestinationDirectoryPath)` | 重命名/移动整个目录 |
| `SaveAsset(AssetToSave, bOnlyIfIsDirty)` | 保存资产到磁盘 |
| `SaveDirectory(DirectoryPath, bOnlyIfIsDirty, bRecursive)` | 保存目录中所有资产 |

##### 示例：复制目录

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"DuplicateDirectory","params":{"SourceDirectoryPath":"/Game/Materials","DestinationDirectoryPath":"/Game/Materials_Backup"}}
```

##### 示例：重命名目录

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"RenameDirectory","params":{"SourceDirectoryPath":"/Game/OldFolder","DestinationDirectoryPath":"/Game/NewFolder"}}
```

#### 删除与引用替换

| 函数 | 说明 |
|------|------|
| `DeleteAsset(AssetPathToDelete)` | 按路径删除资产 |
| `ConsolidateAssets(AssetToConsolidateTo, AssetsToConsolidate)` | 将 `AssetsToConsolidate` 的所有引用替换为 `AssetToConsolidateTo`，然后删除被合并的资产 |

##### 示例：合并资产

将 `M_OldMaterial` 的所有引用替换为 `M_NewMaterial`：

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"ConsolidateAssets","params":{"AssetToConsolidateTo":"/Game/Materials/M_NewMaterial.M_NewMaterial","AssetsToConsolidate":["/Game/Materials/M_OldMaterial.M_OldMaterial"]}}
```

合并后运行 `FixupReferencers` 清理残留重定向器：

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"FixupReferencers","params":{"AssetPaths":["/Game/Materials/M_OldMaterial"]}}
```

#### 目录管理

| 函数 | 说明 |
|------|------|
| `MakeDirectory(DirectoryPath)` | 创建内容目录 |
| `DeleteDirectory(DirectoryPath)` | 删除内容目录 |
| `DoesDirectoryExist(DirectoryPath)` | 检查目录是否存在 |
| `DoesDirectoryHaveAssets(DirectoryPath, bRecursive)` | 检查目录是否包含资产 |

#### 元数据标签

| 函数 | 说明 |
|------|------|
| `SetMetadataTag(Object, Tag, Value)` | 设置资产元数据标签 |
| `GetMetadataTag(Object, Tag)` | 获取资产元数据标签 |
| `RemoveMetadataTag(Object, Tag)` | 移除资产元数据标签 |
| `GetMetadataTagValues(Object)` | 获取资产的所有元数据标签键值对 |

##### 示例：获取所有元数据标签

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"GetMetadataTagValues","params":{"Object":"/Game/Materials/M_Example.M_Example"}}
```

#### 内容浏览器同步

| 函数 | 说明 |
|------|------|
| `SyncBrowserToObjects(AssetPaths)` | 同步内容浏览器以显示指定资产 |

##### 示例：同步浏览器到资产

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"SyncBrowserToObjects","params":{"AssetPaths":["/Game/Materials/M_Example"]}}
```

#### 签出（版本控制）

| 函数 | 说明 |
|------|------|
| `CheckoutAsset(AssetToCheckout)` | 签出资产以编辑 |
| `CheckoutLoadedAsset(AssetToCheckout)` | 签出已加载的资产 |
| `CheckoutDirectory(DirectoryPath, bRecursive)` | 签出目录中所有资产 |

### UEditorUtilityLibrary — 内容浏览器与选择

**CDO 路径**：`/Script/Blutility.Default__EditorUtilityLibrary`

| 函数 | 说明 |
|------|------|
| `GetSelectedAssets()` | 获取内容浏览器中当前选中的资产 |
| `GetSelectedAssetData()` | 获取选中资产的元数据 |
| `GetSelectedAssetsOfClass(AssetClass)` | 按类过滤获取选中资产 |
| `GetCurrentContentBrowserPath()` | 获取内容浏览器当前显示的路径 |
| `GetSelectedFolderPaths()` | 获取内容浏览器中选中的文件夹路径 |
| `SyncBrowserToFolders(FolderList)` | 导航内容浏览器到指定文件夹 |

##### 示例：获取指定类的选中资产

```json
{"object":"/Script/Blutility.Default__EditorUtilityLibrary","function":"GetSelectedAssetsOfClass","params":{"AssetClass":"/Script/Engine.Material"}}
```

##### 示例：导航内容浏览器到文件夹

```json
{"object":"/Script/Blutility.Default__EditorUtilityLibrary","function":"SyncBrowserToFolders","params":{"FolderList":["/Game/Materials"]}}
```

##### 示例：获取当前内容浏览器路径

```json
{"object":"/Script/Blutility.Default__EditorUtilityLibrary","function":"GetCurrentContentBrowserPath"}
```

### UAssetEditorSubsystem

**CDO 路径**：通过 `call` 在子系统上调用（通过 `FindObjectInstances` 获取实例）

| 函数 | 说明 |
|------|------|
| `OpenEditorForAsset(Asset)` | 打开资产编辑器 |
| `CloseAllEditorsForAsset(Asset)` | 关闭资产的所有编辑器 |

## 常用模式

### 浏览内容目录

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"ListAssets","params":{"DirectoryPath":"/Game/Materials","bRecursive":true,"bIncludeFolder":false}}
```

### 复制并重命名资产

先复制再重命名（分步调用）：

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"DuplicateAsset","params":{"SourceAssetPath":"/Game/Materials/M_Base","DestinationAssetPath":"/Game/Materials/M_BaseCopy"}}
```

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"RenameAsset","params":{"SourceAssetPath":"/Game/Materials/M_BaseCopy","DestinationAssetPath":"/Game/Materials/M_NewName"}}
```

### 删除资产

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"DeleteAsset","params":{"AssetPathToDelete":"/Game/Materials/M_Unused"}}
```

### 强制删除资产（忽略引用）

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"ForceDeleteAssets","params":{"AssetPaths":["/Game/OldAssets/M_Deprecated"]}}
```

### 通过 AssetRegistry 查询依赖

先获取 Registry，再调用 GetDependencies（分步调用）：

```json
{"object":"/Script/AssetRegistry.Default__AssetRegistryHelpers","function":"GetAssetRegistry"}
```

然后在返回的 Registry 路径上：

```json
{"object":"<returned_registry_path>","function":"GetDependencies","params":{"PackageName":"/Game/Materials/M_Example","DependencyOptions":{},"OutDependencies":[]}}
```

### 查找项目中所有材质

```json
{"object":"/Script/AssetRegistry.Default__AssetRegistryHelpers","function":"GetAssetRegistry"}
```

然后在返回的 Registry 上：

```json
{"object":"<registry_path>","function":"GetAssetsByClass","params":{"ClassPathName":"/Script/Engine.Material","OutAssetData":[],"bSearchSubClasses":false}}
```

### 保存目录中所有脏资产

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"SaveDirectory","params":{"DirectoryPath":"/Game/Materials","bOnlyIfIsDirty":true,"bRecursive":true}}
```

### 合并并清理

将旧资产的引用替换为规范资产，然后修复重定向器：

```json
{"object":"/Script/EditorScriptingUtilities.Default__EditorAssetLibrary","function":"ConsolidateAssets","params":{"AssetToConsolidateTo":"/Game/Materials/M_Master.M_Master","AssetsToConsolidate":["/Game/Materials/M_Duplicate1.M_Duplicate1","/Game/Materials/M_Duplicate2.M_Duplicate2"]}}
```

```json
{"object":"/Script/UnrealClientProtocolEditor.Default__AssetEditorOperationLibrary","function":"FixupReferencers","params":{"AssetPaths":["/Game/Materials/M_Duplicate1","/Game/Materials/M_Duplicate2"]}}
```

## Changelog

- 1.0.0 (2026-03-29): Initial versioned release

## Known Issues

(none)