// MIT License - Copyright (c) 2025 Italink

#include "LiveCoding/LiveCodingOperationLibrary.h"

#if PLATFORM_WINDOWS
#include "ILiveCodingModule.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLiveCodingOp, Log, All);

#if PLATFORM_WINDOWS
static ILiveCodingModule* GetLiveCodingModule()
{
	return FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
}
#endif

FUCPDeferredResponse ULiveCodingOperationLibrary::Compile()
{
	FUCPDeferredResponse Deferred;

#if PLATFORM_WINDOWS
	ILiveCodingModule* LC = GetLiveCodingModule();
	if (!LC)
	{
		UE_LOG(LogLiveCodingOp, Error, TEXT("Compile: LiveCoding module not loaded"));
		auto Err = MakeShared<FJsonObject>();
		Err->SetStringField(TEXT("error"), TEXT("LiveCoding module not loaded"));
		Deferred.Complete(Err);
		return Deferred;
	}

	if (!LC->IsEnabledForSession())
	{
		UE_LOG(LogLiveCodingOp, Error, TEXT("Compile: LiveCoding not enabled for this session"));
		auto Err = MakeShared<FJsonObject>();
		Err->SetStringField(TEXT("error"), TEXT("LiveCoding not enabled for this session"));
		Deferred.Complete(Err);
		return Deferred;
	}

	if (LC->IsCompiling())
	{
		UE_LOG(LogLiveCodingOp, Warning, TEXT("Compile: A compile is already in progress"));
		auto Err = MakeShared<FJsonObject>();
		Err->SetStringField(TEXT("error"), TEXT("A compile is already in progress"));
		Deferred.Complete(Err);
		return Deferred;
	}

	if (Deferred.IsDeferred())
	{
		FUCPDeferredResponse Captured = Deferred;
		FDelegateHandle Handle;
		Handle = LC->GetOnPatchCompleteDelegate().AddLambda([Captured, Handle]() mutable
		{
			auto Result = MakeShared<FJsonObject>();
			Result->SetStringField(TEXT("status"), TEXT("success"));
			Captured.Complete(Result);

			ILiveCodingModule* LCInner = GetLiveCodingModule();
			if (LCInner)
			{
				LCInner->GetOnPatchCompleteDelegate().Remove(Handle);
			}
		});
		LC->Compile();
	}
	else
	{
		ELiveCodingCompileResult Result = ELiveCodingCompileResult::Failure;
		LC->Compile(ELiveCodingCompileFlags::WaitForCompletion, &Result);
	}
#else
	UE_LOG(LogLiveCodingOp, Error, TEXT("Compile: LiveCoding is only available on Windows"));
	auto Err = MakeShared<FJsonObject>();
	Err->SetStringField(TEXT("error"), TEXT("LiveCoding is only available on Windows"));
	Deferred.Complete(Err);
#endif

	return Deferred;
}

bool ULiveCodingOperationLibrary::IsCompiling()
{
#if PLATFORM_WINDOWS
	ILiveCodingModule* LC = GetLiveCodingModule();
	return LC && LC->IsCompiling();
#else
	return false;
#endif
}

bool ULiveCodingOperationLibrary::IsLiveCodingEnabled()
{
#if PLATFORM_WINDOWS
	ILiveCodingModule* LC = GetLiveCodingModule();
	return LC && LC->IsEnabledForSession();
#else
	return false;
#endif
}

bool ULiveCodingOperationLibrary::EnableLiveCoding(bool bEnable)
{
#if PLATFORM_WINDOWS
	ILiveCodingModule* LC = GetLiveCodingModule();
	if (!LC)
	{
		UE_LOG(LogLiveCodingOp, Error, TEXT("EnableLiveCoding: LiveCoding module not loaded"));
		return false;
	}

	if (bEnable && !LC->CanEnableForSession())
	{
		UE_LOG(LogLiveCodingOp, Error, TEXT("EnableLiveCoding: Cannot enable - %s"), *LC->GetEnableErrorText().ToString());
		return false;
	}

	LC->EnableForSession(bEnable);
	return LC->IsEnabledForSession() == bEnable;
#else
	UE_LOG(LogLiveCodingOp, Error, TEXT("EnableLiveCoding: LiveCoding is only available on Windows"));
	return false;
#endif
}
