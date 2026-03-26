// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Misc/OutputDevice.h"

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
	TSharedPtr<FJsonObject> HandleRequest(const TSharedPtr<FJsonObject>& Request, uint32 ConnectionId = 0);

	static TSharedPtr<FJsonObject> MakeError(const FString& Id, const FString& Error);

private:
	TSharedPtr<FJsonObject> CallUFunction(const TSharedPtr<FJsonObject>& Request, uint32 ConnectionId = 0, const FString& RequestId = FString());

	void BeginLogCapture(ELogVerbosity::Type InMinVerbosity);
	void EndLogCapture(TSharedPtr<FJsonObject>& Response);

	static ELogVerbosity::Type ParseLogLevel(const TSharedPtr<FJsonObject>& Request);

	FUCPLogCapture LogCapture;
	int32 LogCaptureDepth = 0;
};
