// MIT License - Copyright (c) 2025 Italink

#include "UCPServer.h"
#include "UCPRequestHandler.h"
#include "UCPSettings.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"

static constexpr int32 MaxMessageSize = 16 * 1024 * 1024;

FUCPServer* FUCPServer::Instance = nullptr;

FUCPServer* FUCPServer::Get()
{
	return Instance;
}

FUCPServer::FUCPServer(int32 InPort)
	: Port(InPort)
{
	Instance = this;
}

FUCPServer::~FUCPServer()
{
	Stop();
	if (Instance == this)
	{
		Instance = nullptr;
	}
}

void FUCPServer::EnqueueDeferredResponse(uint32 ConnectionId, TSharedPtr<FJsonObject> Json)
{
	FUCPResponse Resp;
	Resp.ConnectionId = ConnectionId;
	Resp.Json = Json;
	PendingResponses.Enqueue(Resp);
}

bool FUCPServer::Start()
{
	RequestHandler = MakeUnique<FUCPRequestHandler>();

	const UUCPSettings* Settings = GetDefault<UUCPSettings>();
	FIPv4Address BindAddr = Settings->bLoopbackOnly ? FIPv4Address(127, 0, 0, 1) : FIPv4Address::Any;
	FIPv4Endpoint Endpoint(BindAddr, Port);

	Listener = MakeUnique<FTcpListener>(Endpoint, FTimespan::FromMilliseconds(100));
	Listener->OnConnectionAccepted().BindRaw(this, &FUCPServer::OnConnectionAccepted);

	return true;
}

void FUCPServer::Stop()
{
	Listener.Reset();

	FScopeLock Lock(&ConnectionsMutex);
	for (auto& Conn : Connections)
	{
		if (Conn.Socket)
		{
			Conn.Socket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Conn.Socket);
			Conn.Socket = nullptr;
		}
	}
	Connections.Empty();

	RequestHandler.Reset();
}

bool FUCPServer::OnConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& InEndpoint)
{
	const UUCPSettings* Settings = GetDefault<UUCPSettings>();
	if (Settings->bLoopbackOnly)
	{
		FIPv4Address RemoteAddr = InEndpoint.Address;
		if (RemoteAddr != FIPv4Address(127, 0, 0, 1))
		{
			UE_LOG(LogTemp, Warning, TEXT("[UCP] Rejected non-loopback connection from %s"), *InEndpoint.ToString());
			return false;
		}
	}

	InSocket->SetNonBlocking(true);
	InSocket->SetNoDelay(true);

	FScopeLock Lock(&ConnectionsMutex);
	uint32 ConnId = NextConnectionId++;
	Connections.Emplace(ConnId, InSocket, InEndpoint);
	UE_LOG(LogTemp, Log, TEXT("[UCP] Client connected from %s (id=%u)"), *InEndpoint.ToString(), ConnId);
	return true;
}

void FUCPServer::Tick()
{
	PollConnections();

	FUCPRequest Req;
	while (PendingRequests.Dequeue(Req))
	{
		TSharedPtr<FJsonObject> ResponseJson = RequestHandler->HandleRequest(Req.Json, Req.ConnectionId);

		if (ResponseJson.IsValid())
		{
			FUCPResponse Resp;
			Resp.ConnectionId = Req.ConnectionId;
			Resp.Json = ResponseJson;
			PendingResponses.Enqueue(Resp);
		}
	}

	FlushResponses();
	CleanupDeadConnections();
}

void FUCPServer::PollConnections()
{
	FScopeLock Lock(&ConnectionsMutex);

	for (auto& Conn : Connections)
	{
		if (!Conn.Socket || Conn.bPendingKill)
		{
			continue;
		}

		uint32 PendingSize = 0;
		if (Conn.Socket->HasPendingData(PendingSize) && PendingSize > 0)
		{
			int32 OldSize = Conn.RecvBuffer.Num();
			Conn.RecvBuffer.AddUninitialized(PendingSize);

			int32 BytesRead = 0;
			if (!Conn.Socket->Recv(Conn.RecvBuffer.GetData() + OldSize, PendingSize, BytesRead))
			{
				UE_LOG(LogTemp, Warning, TEXT("[UCP] Recv failed for connection %u, closing."), Conn.Id);
				Conn.bPendingKill = true;
				continue;
			}

			if (BytesRead <= 0)
			{
				Conn.RecvBuffer.SetNum(OldSize);
			}
			else
			{
				Conn.RecvBuffer.SetNum(OldSize + BytesRead);
			}
		}

		ESocketConnectionState State = Conn.Socket->GetConnectionState();
		if (State == SCS_ConnectionError)
		{
			UE_LOG(LogTemp, Log, TEXT("[UCP] Connection %u lost."), Conn.Id);
			Conn.bPendingKill = true;
			continue;
		}

		ProcessRecvBuffer(Conn);
	}
}

void FUCPServer::ProcessRecvBuffer(FUCPClientConnection& Conn)
{
	while (Conn.RecvBuffer.Num() >= 4)
	{
		uint32 MsgLen = 0;
		FMemory::Memcpy(&MsgLen, Conn.RecvBuffer.GetData(), 4);

		if (MsgLen > (uint32)MaxMessageSize)
		{
			UE_LOG(LogTemp, Error, TEXT("[UCP] Message too large (%u bytes) from connection %u, closing."), MsgLen, Conn.Id);
			Conn.bPendingKill = true;
			return;
		}

		if ((uint32)Conn.RecvBuffer.Num() < 4 + MsgLen)
		{
			break;
		}

		FString JsonString;
		{
			FUTF8ToTCHAR Converter(
				reinterpret_cast<const ANSICHAR*>(Conn.RecvBuffer.GetData() + 4),
				MsgLen
			);
			JsonString = FString(Converter.Length(), Converter.Get());
		}

		Conn.RecvBuffer.RemoveAt(0, 4 + MsgLen);

		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[UCP] Invalid JSON from connection %u"), Conn.Id);

			TSharedPtr<FJsonObject> ErrJson = MakeShared<FJsonObject>();
			ErrJson->SetBoolField(TEXT("success"), false);
			ErrJson->SetStringField(TEXT("error"), TEXT("Invalid JSON"));
			SendJsonToConnection(Conn, ErrJson);
			continue;
		}

		FUCPRequest Req;
		Req.ConnectionId = Conn.Id;
		Req.Json = JsonObj;
		PendingRequests.Enqueue(Req);
	}
}

void FUCPServer::FlushResponses()
{
	FScopeLock Lock(&ConnectionsMutex);

	FUCPResponse Resp;
	while (PendingResponses.Dequeue(Resp))
	{
		FUCPClientConnection* Conn = FindConnectionById(Resp.ConnectionId);
		if (Conn)
		{
			SendJsonToConnection(*Conn, Resp.Json);
		}
	}
}

void FUCPServer::SendJsonToConnection(FUCPClientConnection& Conn, const TSharedPtr<FJsonObject>& Json)
{
	if (!Conn.Socket || Conn.bPendingKill)
	{
		return;
	}

	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	FTCHARToUTF8 Utf8(*JsonString);
	uint32 BodyLen = (uint32)Utf8.Length();

	int32 BytesSent = 0;
	Conn.Socket->Send(reinterpret_cast<const uint8*>(&BodyLen), 4, BytesSent);
	Conn.Socket->Send(reinterpret_cast<const uint8*>(Utf8.Get()), BodyLen, BytesSent);
}

void FUCPServer::CleanupDeadConnections()
{
	FScopeLock Lock(&ConnectionsMutex);

	for (int32 i = Connections.Num() - 1; i >= 0; --i)
	{
		if (Connections[i].bPendingKill)
		{
			if (Connections[i].Socket)
			{
				Connections[i].Socket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connections[i].Socket);
			}
			UE_LOG(LogTemp, Log, TEXT("[UCP] Connection %u cleaned up."), Connections[i].Id);
			Connections.RemoveAt(i);
		}
	}
}

FUCPClientConnection* FUCPServer::FindConnectionById(uint32 Id)
{
	for (auto& Conn : Connections)
	{
		if (Conn.Id == Id)
		{
			return &Conn;
		}
	}
	return nullptr;
}
