// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetEditorOperationLibrary.generated.h"

UCLASS()
class UAssetEditorOperationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static TScriptInterface<IAssetRegistry> GetAssetRegistry();

	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static int32 ForceDeleteAssets(const TArray<FString>& AssetPaths);

	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static bool FixupReferencers(const TArray<FString>& AssetPaths);

	/** Create a Blueprint asset. ParentClassPath e.g. "/Script/Engine.Actor". BlueprintType: "Normal","Const","MacroLibrary","Interface","FunctionLibrary". */
	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static UBlueprint* CreateBlueprint(const FString& AssetName, const FString& PackagePath, const FString& ParentClassPath, const FString& BlueprintType = TEXT("Normal"));

	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static bool CompileBlueprint(const FString& BlueprintPath);

	UFUNCTION(BlueprintCallable, Category = "UCP|Asset|Editor")
	static bool CanCreateBlueprintOfClass(const FString& ClassPath);
};
