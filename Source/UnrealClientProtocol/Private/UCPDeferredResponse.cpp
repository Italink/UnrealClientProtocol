// MIT License - Copyright (c) 2025 Italink

#include "UCPDeferredResponse.h"
#include "UCPServer.h"

TArray<FUCPCallContext>& FUCPCallContext::GetStack()
{
	static TArray<FUCPCallContext> Stack;
	return Stack;
}

bool FUCPCallContext::HasContext()
{
	return GetStack().Num() > 0;
}

const FUCPCallContext& FUCPCallContext::Current()
{
	return GetStack().Last();
}

FUCPCallScope::FUCPCallScope(uint32 InConnectionId, const FString& InRequestId)
{
	FUCPCallContext Ctx;
	Ctx.ConnectionId = InConnectionId;
	Ctx.RequestId = InRequestId;
	FUCPCallContext::GetStack().Push(MoveTemp(Ctx));
}

FUCPCallScope::~FUCPCallScope()
{
	FUCPCallContext::GetStack().Pop();
}

FUCPDeferredResponse::FUCPDeferredResponse()
{
	if (FUCPCallContext::HasContext())
	{
		const FUCPCallContext& Ctx = FUCPCallContext::Current();
		ConnectionId = static_cast<int32>(Ctx.ConnectionId);
		RequestId = Ctx.RequestId;
	}
}

void FUCPDeferredResponse::Complete(TSharedPtr<FJsonObject> Result) const
{
	if (FUCPServer* Server = FUCPServer::Get())
	{
		TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
		Response->SetStringField(TEXT("id"), RequestId);
		Response->SetBoolField(TEXT("success"), true);
		if (Result.IsValid())
		{
			Response->SetObjectField(TEXT("result"), Result);
		}
		Server->EnqueueDeferredResponse(static_cast<uint32>(ConnectionId), Response);
	}
}
