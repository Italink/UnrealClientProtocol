// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "UCPDeferredResponse.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LiveCodingOperationLibrary.generated.h"

UCLASS()
class ULiveCodingOperationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UCP|LiveCoding")
	static FUCPDeferredResponse Compile();

	UFUNCTION(BlueprintCallable, Category = "UCP|LiveCoding")
	static bool IsCompiling();

	UFUNCTION(BlueprintCallable, Category = "UCP|LiveCoding")
	static bool IsLiveCodingEnabled();

	UFUNCTION(BlueprintCallable, Category = "UCP|LiveCoding")
	static bool EnableLiveCoding(bool bEnable);
};
