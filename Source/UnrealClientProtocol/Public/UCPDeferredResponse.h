// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "UCPDeferredResponse.generated.h"

struct UNREALCLIENTPROTOCOL_API FUCPCallContext
{
	uint32 ConnectionId = 0;
	FString RequestId;

	static TArray<FUCPCallContext>& GetStack();
	static bool HasContext();
	static const FUCPCallContext& Current();
};

struct UNREALCLIENTPROTOCOL_API FUCPCallScope
{
	FUCPCallScope(uint32 InConnectionId, const FString& InRequestId);
	~FUCPCallScope();
	FUCPCallScope(const FUCPCallScope&) = delete;
	FUCPCallScope& operator=(const FUCPCallScope&) = delete;
};

USTRUCT(BlueprintType)
struct UNREALCLIENTPROTOCOL_API FUCPDeferredResponse
{
	GENERATED_BODY()

	FUCPDeferredResponse();

	UPROPERTY()
	int32 ConnectionId = 0;

	UPROPERTY()
	FString RequestId;

	bool IsDeferred() const { return ConnectionId > 0; }

	void Complete(TSharedPtr<FJsonObject> Result) const;
};
