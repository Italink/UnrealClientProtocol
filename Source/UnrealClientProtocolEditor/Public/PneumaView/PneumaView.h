// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PneumaView.generated.h"

class FPneumaViewEditor;
class UPneumaViewConfig;

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

	UPROPERTY(BlueprintReadOnly, Category = "UCP|PneumaView")
	FString UniqueName;

private:
	static TMap<FString, TObjectPtr<UPneumaView>> Instances;

	TSharedPtr<FPneumaViewEditor> Editor;
	TObjectPtr<UPneumaViewConfig> Config;

	void OnEditorClosed();
};
