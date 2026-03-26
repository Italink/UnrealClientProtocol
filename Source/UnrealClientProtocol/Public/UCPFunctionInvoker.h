// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FUCPFunctionInvoker
{
public:
	static TSharedPtr<FJsonObject> Invoke(
		const FString& ObjectPath,
		const FString& FunctionName,
		const TSharedPtr<FJsonObject>& ParamsJson,
		uint32 ConnectionId = 0,
		const FString& RequestId = FString());

	static TSharedPtr<FJsonObject> DescribeObject(const FString& ObjectPath);

	static TSharedPtr<FJsonObject> DescribeProperty(
		const FString& ObjectPath,
		const FString& PropertyName);

	static TSharedPtr<FJsonObject> DescribeFunction(
		const FString& ObjectPath,
		const FString& FunctionName);

private:
	static UObject* FindTargetObject(const FString& ObjectPath, FString& OutError);
	static UFunction* FindTargetFunction(UObject* Obj, const FString& FuncName, FString& OutError);
	static TSharedPtr<FJsonObject> MakeErrorResponse(const FString& Id, const FString& Error);
	static TSharedPtr<FJsonObject> MakeSuccessResponse(const FString& Id, const TSharedPtr<FJsonObject>& Result);
	static FString GetPropertyTypeString(FProperty* Prop);
};
