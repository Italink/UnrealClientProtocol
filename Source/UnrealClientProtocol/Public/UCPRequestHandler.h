// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Misc/OutputDevice.h"

DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<FJsonObject>, FUCPCommandDelegate, const TSharedPtr<FJsonObject>&);

class FUCPLogCapture : public FOutputDevice
{
public:
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

	TArray<FString> Entries;
	ELogVerbosity::Type MinVerbosity = ELogVerbosity::Warning;
	bool bHasIssues = false;

	void Reset(ELogVerbosity::Type InMinVerbosity);
};

class UNREALCLIENTPROTOCOL_API FUCPRequestHandler
{
public:
	TSharedPtr<FJsonObject> HandleRequest(const TSharedPtr<FJsonObject>& Request);

	void RegisterCommand(const FString& CommandType, FUCPCommandDelegate Handler);
	void UnregisterCommand(const FString& CommandType);

	static TSharedPtr<FJsonObject> MakeError(const FString& Id, const FString& Error);

private:
	TSharedPtr<FJsonObject> DispatchSingle(const TSharedPtr<FJsonObject>& Request);
	TSharedPtr<FJsonObject> ExecBatch(const TSharedPtr<FJsonObject>& Request);
	TSharedPtr<FJsonObject> CallUFunction(const TSharedPtr<FJsonObject>& Request);

	void CopyIdField(const TSharedPtr<FJsonObject>& From, const TSharedPtr<FJsonObject>& To);

	void BeginLogCapture(ELogVerbosity::Type InMinVerbosity);
	void EndLogCapture(TSharedPtr<FJsonObject>& Response);

	static ELogVerbosity::Type ParseLogLevel(const TSharedPtr<FJsonObject>& Request);

	TMap<FString, FUCPCommandDelegate> ExternalCommands;
	FUCPLogCapture LogCapture;
	int32 LogCaptureDepth = 0;
};
