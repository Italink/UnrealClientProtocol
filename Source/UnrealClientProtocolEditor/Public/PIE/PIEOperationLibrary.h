// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PIEOperationLibrary.generated.h"

UCLASS()
class UPIEOperationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool StartPIE();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool StartSimulate();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static void StopPIE();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool IsInPIE();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool IsSimulating();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool PausePIE();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static bool ResumePIE();

	UFUNCTION(BlueprintCallable, Category = "UCP|PIE")
	static TArray<UWorld*> GetPIEWorlds();
};
