// MIT License - Copyright (c) 2025 Italink

#include "PIE/PIEOperationLibrary.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogPIEOp, Log, All);

bool UPIEOperationLibrary::StartPIE()
{
	if (!GEditor)
	{
		UE_LOG(LogPIEOp, Error, TEXT("StartPIE: GEditor not available"));
		return false;
	}

	if (GEditor->IsPlaySessionInProgress())
	{
		UE_LOG(LogPIEOp, Warning, TEXT("StartPIE: A play session is already in progress"));
		return false;
	}

	FRequestPlaySessionParams Params;
	Params.SessionDestination = EPlaySessionDestinationType::InProcess;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	GEditor->RequestPlaySession(Params);
	return true;
}

bool UPIEOperationLibrary::StartSimulate()
{
	if (!GEditor)
	{
		UE_LOG(LogPIEOp, Error, TEXT("StartSimulate: GEditor not available"));
		return false;
	}

	if (GEditor->IsPlaySessionInProgress())
	{
		UE_LOG(LogPIEOp, Warning, TEXT("StartSimulate: A play session is already in progress"));
		return false;
	}

	FRequestPlaySessionParams Params;
	Params.SessionDestination = EPlaySessionDestinationType::InProcess;
	Params.WorldType = EPlaySessionWorldType::SimulateInEditor;
	GEditor->RequestPlaySession(Params);
	return true;
}

void UPIEOperationLibrary::StopPIE()
{
	if (!GEditor)
	{
		UE_LOG(LogPIEOp, Error, TEXT("StopPIE: GEditor not available"));
		return;
	}

	if (!GEditor->IsPlaySessionInProgress())
	{
		UE_LOG(LogPIEOp, Warning, TEXT("StopPIE: No play session in progress"));
		return;
	}

	GEditor->RequestEndPlayMap();
}

bool UPIEOperationLibrary::IsInPIE()
{
	return GEditor && GEditor->IsPlaySessionInProgress();
}

bool UPIEOperationLibrary::IsSimulating()
{
	return GEditor && GEditor->IsSimulateInEditorInProgress();
}

bool UPIEOperationLibrary::PausePIE()
{
	if (!GEditor || !GEditor->IsPlaySessionInProgress())
	{
		return false;
	}
	return GEditor->SetPIEWorldsPaused(true);
}

bool UPIEOperationLibrary::ResumePIE()
{
	if (!GEditor || !GEditor->IsPlaySessionInProgress())
	{
		return false;
	}
	return GEditor->SetPIEWorldsPaused(false);
}

TArray<UWorld*> UPIEOperationLibrary::GetPIEWorlds()
{
	TArray<UWorld*> Result;
	if (!GEngine)
	{
		return Result;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			Result.Add(Context.World());
		}
	}
	return Result;
}
