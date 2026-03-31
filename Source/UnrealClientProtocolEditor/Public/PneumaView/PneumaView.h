// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PneumaView.generated.h"

class FPneumaViewEditor;
class UPneumaViewConfig;
class FAdvancedPreviewScene;

USTRUCT(BlueprintType)
struct FPneumaCameraPose
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PneumaView")
	FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS(BlueprintType)
class UPneumaView : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	static UPneumaView* FindOrCreate(const FString& UniqueName, const FString& ConfigType);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	bool Close();

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	bool SetViewMode(const FString& ModeName);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	FString GetViewMode();

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	TArray<FString> CaptureViewport(const TArray<FPneumaCameraPose>& CameraPoses, const FString& CaptureConfigType);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	FString GetPreviewWorld();

	// --- Preview scene actor management ---

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	AActor* SpawnPreviewMeshActor(UStaticMesh* Mesh,
		FVector Location = FVector::ZeroVector,
		FRotator Rotation = FRotator::ZeroRotator,
		FVector Scale = FVector(1.0,1.0,1.0));

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	bool DestroyPreviewActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	bool SetActorMaterial(AActor* Actor, UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	bool SetActorStaticMesh(AActor* Actor, UStaticMesh* Mesh);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	void FocusOnActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "UCP|PneumaView")
	void FocusOnAll();

	// --- GC protection ---

	void AddManagedObject(UObject* Object);
	void RemoveManagedObject(UObject* Object);

	UPROPERTY(BlueprintReadOnly, Category = "UCP|PneumaView")
	FString UniqueName;

	TSharedPtr<FPneumaViewEditor> GetEditor() const { return Editor; }
	UPneumaViewConfig* GetConfig() const { return Config; }

protected:
	static TMap<FString, TObjectPtr<UPneumaView>> Instances;

	UWorld* GetPreviewWorldPtr() const;
	TSharedPtr<FAdvancedPreviewScene> GetPreviewScenePtr() const;

	void InitFromConfig(const FString& InUniqueName, UPneumaViewConfig* InConfig);

	TSharedPtr<FPneumaViewEditor> Editor;

	UPROPERTY()
	TObjectPtr<UPneumaViewConfig> Config;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> ManagedObjects;

	virtual void OnEditorClosed();
	void CleanupManagedObjects();
};
