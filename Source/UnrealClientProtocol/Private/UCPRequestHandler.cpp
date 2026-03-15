// MIT License - Copyright (c) 2025 Italink

#include "UCPRequestHandler.h"
#include "UCPFunctionInvoker.h"
#include "UCPSettings.h"
#include "UObject/UObjectGlobals.h"
#include "Dom/JsonValue.h"
#include "Misc/OutputDeviceRedirector.h"

// ---- Log Capture ----

void FUCPLogCapture::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	if (Verbosity <= MinVerbosity)
	{
		FString LevelStr;
		switch (Verbosity)
		{
		case ELogVerbosity::Fatal:   LevelStr = TEXT("Fatal"); break;
		case ELogVerbosity::Error:   LevelStr = TEXT("Error"); break;
		case ELogVerbosity::Warning: LevelStr = TEXT("Warning"); break;
		case ELogVerbosity::Display: LevelStr = TEXT("Display"); break;
		case ELogVerbosity::Log:     LevelStr = TEXT("Log"); break;
		default:                     LevelStr = TEXT("Verbose"); break;
		}

		Entries.Add(FString::Printf(TEXT("[%s] %s: %s"), *LevelStr, *Category.ToString(), V));

		if (Verbosity <= ELogVerbosity::Warning)
		{
			bHasIssues = true;
		}
	}
}

void FUCPLogCapture::Reset(ELogVerbosity::Type InMinVerbosity)
{
	Entries.Empty();
	MinVerbosity = InMinVerbosity;
	bHasIssues = false;
}

// ---- Security ----

static bool IsCallAllowed(const FString& ObjectPath, const FString& FunctionName)
{
	const UUCPSettings* Settings = GetDefault<UUCPSettings>();

	for (const FString& Blocked : Settings->BlockedFunctions)
	{
		if (Blocked.Equals(FunctionName, ESearchCase::IgnoreCase))
		{
			return false;
		}
		if (Blocked.Contains(TEXT("::")) && ObjectPath.Contains(Blocked.Left(Blocked.Find(TEXT("::")))))
		{
			FString BlockedFunc = Blocked.Mid(Blocked.Find(TEXT("::")) + 2);
			if (BlockedFunc.Equals(FunctionName, ESearchCase::IgnoreCase))
			{
				return false;
			}
		}
	}

	if (Settings->AllowedClassPrefixes.Num() > 0)
	{
		bool bAllowed = false;
		for (const FString& Prefix : Settings->AllowedClassPrefixes)
		{
			if (ObjectPath.StartsWith(Prefix))
			{
				bAllowed = true;
				break;
			}
		}
		if (!bAllowed)
		{
			return false;
		}
	}

	return true;
}

// ---- Log Capture Nesting ----

ELogVerbosity::Type FUCPRequestHandler::ParseLogLevel(const TSharedPtr<FJsonObject>& Request)
{
	if (Request->HasField(TEXT("log_level")))
	{
		FString Level = Request->GetStringField(TEXT("log_level"));
		if (Level == TEXT("all") || Level == TEXT("verbose")) return ELogVerbosity::VeryVerbose;
		if (Level == TEXT("log"))     return ELogVerbosity::Log;
		if (Level == TEXT("display")) return ELogVerbosity::Display;
		if (Level == TEXT("warning")) return ELogVerbosity::Warning;
		if (Level == TEXT("error"))   return ELogVerbosity::Error;
	}
	return ELogVerbosity::Warning;
}

void FUCPRequestHandler::BeginLogCapture(ELogVerbosity::Type InMinVerbosity)
{
	LogCaptureDepth++;
	if (LogCaptureDepth == 1)
	{
		LogCapture.Reset(InMinVerbosity);
		FOutputDeviceRedirector::Get()->AddOutputDevice(&LogCapture);
	}
}

void FUCPRequestHandler::EndLogCapture(TSharedPtr<FJsonObject>& Response)
{
	LogCaptureDepth--;
	if (LogCaptureDepth == 0)
	{
		FOutputDeviceRedirector::Get()->RemoveOutputDevice(&LogCapture);

		if (LogCapture.bHasIssues && Response.IsValid())
		{
			TArray<TSharedPtr<FJsonValue>> LogArray;
			for (const FString& Entry : LogCapture.Entries)
			{
				LogArray.Add(MakeShared<FJsonValueString>(Entry));
			}
			Response->SetArrayField(TEXT("log"), LogArray);
		}
	}
}

// ---- Request Handling ----

TSharedPtr<FJsonObject> FUCPRequestHandler::HandleRequest(const TSharedPtr<FJsonObject>& Request)
{
	if (!Request.IsValid())
	{
		return MakeError(FString(), TEXT("Null request"));
	}

	ELogVerbosity::Type RequestLogLevel = ParseLogLevel(Request);
	BeginLogCapture(RequestLogLevel);

	FString Type = Request->GetStringField(TEXT("type"));

	TSharedPtr<FJsonObject> Response;

	if (Type == TEXT("batch"))
	{
		Response = ExecBatch(Request);
		CopyIdField(Request, Response);
	}
	else
	{
		Response = DispatchSingle(Request);
		CopyIdField(Request, Response);
	}

	EndLogCapture(Response);

	return Response;
}

TSharedPtr<FJsonObject> FUCPRequestHandler::DispatchSingle(const TSharedPtr<FJsonObject>& Request)
{
	FString Type = Request->GetStringField(TEXT("type"));

	if (FUCPCommandDelegate* Handler = ExternalCommands.Find(Type))
	{
		return Handler->Execute(Request);
	}

	if (Type == TEXT("call"))
	{
		return CallUFunction(Request);
	}

	return MakeError(FString(), FString::Printf(TEXT("Unknown request type: %s"), *Type));
}

TSharedPtr<FJsonObject> FUCPRequestHandler::ExecBatch(const TSharedPtr<FJsonObject>& Request)
{
	const TArray<TSharedPtr<FJsonValue>>* Commands;
	if (!Request->TryGetArrayField(TEXT("commands"), Commands))
	{
		return MakeError(FString(), TEXT("batch request requires a 'commands' array"));
	}

	TArray<TSharedPtr<FJsonValue>> Results;
	Results.Reserve(Commands->Num());

	for (const TSharedPtr<FJsonValue>& CmdVal : *Commands)
	{
		TSharedPtr<FJsonObject> CmdObj = CmdVal->AsObject();
		if (!CmdObj.IsValid())
		{
			TSharedPtr<FJsonObject> Err = MakeError(FString(), TEXT("Each command in batch must be a JSON object"));
			Results.Add(MakeShared<FJsonValueObject>(Err));
			continue;
		}

		TSharedPtr<FJsonObject> SingleResp = DispatchSingle(CmdObj);
		CopyIdField(CmdObj, SingleResp);
		Results.Add(MakeShared<FJsonValueObject>(SingleResp));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetArrayField(TEXT("results"), Results);
	return Response;
}

TSharedPtr<FJsonObject> FUCPRequestHandler::CallUFunction(const TSharedPtr<FJsonObject>& Request)
{
	FString ObjectPath = Request->GetStringField(TEXT("object"));
	FString FunctionName = Request->GetStringField(TEXT("function"));
	TSharedPtr<FJsonObject> Params;
	if (Request->HasField(TEXT("params")))
	{
		Params = Request->GetObjectField(TEXT("params"));
	}

	if (ObjectPath.IsEmpty())
	{
		return MakeError(FString(), TEXT("Missing 'object' field"));
	}
	if (FunctionName.IsEmpty())
	{
		return MakeError(FString(), TEXT("Missing 'function' field"));
	}

	if (!IsCallAllowed(ObjectPath, FunctionName))
	{
		return MakeError(FString(), FString::Printf(TEXT("Call denied by security policy: %s::%s"), *ObjectPath, *FunctionName));
	}

	TSharedPtr<FJsonObject> Result = FUCPFunctionInvoker::Invoke(ObjectPath, FunctionName, Params);

	if (!Result->GetBoolField(TEXT("success")))
	{
		TSharedPtr<FJsonObject> Sig = FUCPFunctionInvoker::DescribeFunction(ObjectPath, FunctionName);
		if (Sig->GetBoolField(TEXT("success")))
		{
			Result->SetObjectField(TEXT("expected"), Sig->GetObjectField(TEXT("result")));
		}
	}

	return Result;
}

// ---- Registration ----

void FUCPRequestHandler::RegisterCommand(const FString& CommandType, FUCPCommandDelegate Handler)
{
	ExternalCommands.Add(CommandType, Handler);
	UE_LOG(LogTemp, Log, TEXT("[UCP] Registered external command: %s"), *CommandType);
}

void FUCPRequestHandler::UnregisterCommand(const FString& CommandType)
{
	ExternalCommands.Remove(CommandType);
	UE_LOG(LogTemp, Log, TEXT("[UCP] Unregistered external command: %s"), *CommandType);
}

TSharedPtr<FJsonObject> FUCPRequestHandler::MakeError(const FString& Id, const FString& Error)
{
	TSharedPtr<FJsonObject> Resp = MakeShared<FJsonObject>();
	if (!Id.IsEmpty())
	{
		Resp->SetStringField(TEXT("id"), Id);
	}
	Resp->SetBoolField(TEXT("success"), false);
	Resp->SetStringField(TEXT("error"), Error);
	return Resp;
}

void FUCPRequestHandler::CopyIdField(const TSharedPtr<FJsonObject>& From, const TSharedPtr<FJsonObject>& To)
{
	if (From.IsValid() && To.IsValid() && From->HasField(TEXT("id")))
	{
		To->SetStringField(TEXT("id"), From->GetStringField(TEXT("id")));
	}
}
