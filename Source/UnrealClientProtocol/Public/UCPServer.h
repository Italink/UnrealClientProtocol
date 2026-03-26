// MIT License - Copyright (c) 2025 Italink

#pragma once

#include "CoreMinimal.h"
#include "Common/TcpListener.h"

class FUCPRequestHandler;

struct FUCPClientConnection
{
	uint32 Id = 0;
	FSocket* Socket = nullptr;
	FIPv4Endpoint RemoteEndpoint;
	TArray<uint8> RecvBuffer;
	bool bPendingKill = false;

	FUCPClientConnection() = default;
	FUCPClientConnection(uint32 InId, FSocket* InSocket, const FIPv4Endpoint& InEndpoint)
		: Id(InId), Socket(InSocket), RemoteEndpoint(InEndpoint) {}
};

struct FUCPRequest
{
	uint32 ConnectionId = 0;
	TSharedPtr<FJsonObject> Json;
};

struct FUCPResponse
{
	uint32 ConnectionId = 0;
	TSharedPtr<FJsonObject> Json;
};

class UNREALCLIENTPROTOCOL_API FUCPServer
{
public:
	explicit FUCPServer(int32 InPort);
	~FUCPServer();

	bool Start();
	void Stop();
	void Tick();

	static FUCPServer* Get();
	void EnqueueDeferredResponse(uint32 ConnectionId, TSharedPtr<FJsonObject> Json);

private:
	static FUCPServer* Instance;
	bool OnConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& InEndpoint);

	void PollConnections();
	void ProcessRecvBuffer(FUCPClientConnection& Conn);
	void FlushResponses();
	void SendJsonToConnection(FUCPClientConnection& Conn, const TSharedPtr<FJsonObject>& Json);
	void CleanupDeadConnections();

	FUCPClientConnection* FindConnectionById(uint32 Id);

	int32 Port;
	TUniquePtr<FTcpListener> Listener;

	FCriticalSection ConnectionsMutex;
	TArray<FUCPClientConnection> Connections;
	uint32 NextConnectionId = 1;

	TQueue<FUCPRequest, EQueueMode::Mpsc> PendingRequests;
	TQueue<FUCPResponse, EQueueMode::Mpsc> PendingResponses;

	TUniquePtr<FUCPRequestHandler> RequestHandler;
};
