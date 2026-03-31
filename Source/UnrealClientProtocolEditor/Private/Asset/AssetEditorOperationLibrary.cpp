// MIT License - Copyright (c) 2025 Italink

#include "Asset/AssetEditorOperationLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "UObject/UObjectGlobals.h"
#include "Kismet2/KismetEditorUtilities.h"

TScriptInterface<IAssetRegistry> UAssetEditorOperationLibrary::GetAssetRegistry()
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	return TScriptInterface<IAssetRegistry>(Cast<UObject>(&Registry));
}

int32 UAssetEditorOperationLibrary::ForceDeleteAssets(const TArray<FString>& AssetPaths)
{
	TArray<UObject*> ObjectsToDelete;
	for (const FString& Path : AssetPaths)
	{
		UObject* Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *Path);
		if (Obj)
		{
			ObjectsToDelete.Add(Obj);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[UCP] ForceDeleteAssets: Could not load asset: %s"), *Path);
		}
	}

	if (ObjectsToDelete.Num() == 0)
	{
		return 0;
	}

	return ObjectTools::ForceDeleteObjects(ObjectsToDelete, false);
}

bool UAssetEditorOperationLibrary::FixupReferencers(const TArray<FString>& AssetPaths)
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<UObjectRedirector*> Redirectors;
	for (const FString& Path : AssetPaths)
	{
		FAssetData AssetData = Registry.GetAssetByObjectPath(FSoftObjectPath(Path));
		if (AssetData.IsValid() && AssetData.IsRedirector())
		{
			UObject* Obj = AssetData.GetAsset();
			if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(Obj))
			{
				Redirectors.Add(Redirector);
			}
		}
	}

	if (Redirectors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UCP] FixupReferencers: No redirectors found in the provided paths"));
		return false;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.FixupReferencers(Redirectors, false);
	return true;
}

UBlueprint* UAssetEditorOperationLibrary::CreateBlueprint(const FString& AssetName, const FString& PackagePath, const FString& ParentClassPath, const FString& BlueprintType)
{
	UClass* ParentClass = FindObject<UClass>(nullptr, *ParentClassPath);
	if (!ParentClass)
	{
		ParentClass = LoadObject<UClass>(nullptr, *ParentClassPath);
	}
	if (!ParentClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[UCP] CreateBlueprint: ParentClass not found: %s"), *ParentClassPath);
		return nullptr;
	}

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[UCP] CreateBlueprint: Cannot create blueprint of class: %s"), *ParentClassPath);
		return nullptr;
	}

	EBlueprintType BPType = BPTYPE_Normal;
	if (BlueprintType == TEXT("Const"))
	{
		BPType = BPTYPE_Const;
	}
	else if (BlueprintType == TEXT("MacroLibrary"))
	{
		BPType = BPTYPE_MacroLibrary;
	}
	else if (BlueprintType == TEXT("Interface"))
	{
		BPType = BPTYPE_Interface;
	}
	else if (BlueprintType == TEXT("FunctionLibrary"))
	{
		BPType = BPTYPE_FunctionLibrary;
	}

	FString FullPath = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("[UCP] CreateBlueprint: Failed to create package: %s"), *FullPath);
		return nullptr;
	}

	UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(ParentClass, Package, FName(*AssetName), BPType, NAME_None);
	if (NewBP)
	{
		FAssetRegistryModule::AssetCreated(NewBP);
		NewBP->MarkPackageDirty();
	}
	return NewBP;
}

bool UAssetEditorOperationLibrary::CompileBlueprint(const FString& BlueprintPath)
{
	UObject* Obj = StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath);
	UBlueprint* BP = Cast<UBlueprint>(Obj);
	if (!BP)
	{
		UE_LOG(LogTemp, Error, TEXT("[UCP] CompileBlueprint: Blueprint not found: %s"), *BlueprintPath);
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(BP);
	return true;
}

bool UAssetEditorOperationLibrary::CanCreateBlueprintOfClass(const FString& ClassPath)
{
	UClass* Class = FindObject<UClass>(nullptr, *ClassPath);
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassPath);
	}
	if (!Class)
	{
		return false;
	}
	return FKismetEditorUtilities::CanCreateBlueprintOfClass(Class);
}
