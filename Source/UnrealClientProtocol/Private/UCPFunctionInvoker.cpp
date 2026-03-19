// MIT License - Copyright (c) 2025 Italink

#include "UCPFunctionInvoker.h"
#include "UCPParamConverter.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "Dom/JsonValue.h"
#include "Modules/ModuleManager.h"

TSharedPtr<FJsonObject> FUCPFunctionInvoker::Invoke(
	const FString& ObjectPath,
	const FString& FunctionName,
	const TSharedPtr<FJsonObject>& ParamsJson)
{
	FString Error;

	UObject* Obj = FindTargetObject(ObjectPath, Error);
	if (!Obj)
	{
		return MakeErrorResponse(FString(), Error);
	}

	UFunction* Func = FindTargetFunction(Obj, FunctionName, Error);
	if (!Func)
	{
		return MakeErrorResponse(FString(), Error);
	}

	for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (CastField<FStructProperty>(*It))
		{
			FStructProperty* StructProp = CastField<FStructProperty>(*It);
			if (StructProp->Struct && StructProp->Struct->GetFName() == FName(TEXT("LatentActionInfo")))
			{
				return MakeErrorResponse(FString(),
					FString::Printf(TEXT("Latent functions are not supported: %s::%s"),
						*Obj->GetClass()->GetPathName(), *FunctionName));
			}
		}
	}

	const int32 ParmsSize = Func->ParmsSize;
	uint8* ParamBuffer = nullptr;
	TArray<uint8> ParamStorage;

	if (ParmsSize > 0)
	{
		ParamStorage.AddZeroed(ParmsSize);
		ParamBuffer = ParamStorage.GetData();

		for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_ZeroConstructor))
			{
				It->InitializeValue_InContainer(ParamBuffer);
			}
		}

		Error = FUCPParamConverter::JsonToParams(Func, ParamsJson, ParamBuffer, Obj);
		if (!Error.IsEmpty())
		{
			for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
			{
				It->DestroyValue_InContainer(ParamBuffer);
			}
			return MakeErrorResponse(FString(), Error);
		}
	}

	Obj->ProcessEvent(Func, ParamBuffer);

	TSharedPtr<FJsonObject> ResultJson;
	if (ParamBuffer)
	{
		ResultJson = FUCPParamConverter::ParamsToJson(Func, ParamBuffer);

		for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->DestroyValue_InContainer(ParamBuffer);
		}
	}

	return MakeSuccessResponse(FString(), ResultJson);
}

TSharedPtr<FJsonObject> FUCPFunctionInvoker::DescribeObject(const FString& ObjectPath)
{
	FString Error;
	UObject* Obj = FindTargetObject(ObjectPath, Error);
	if (!Obj)
	{
		return MakeErrorResponse(FString(), Error);
	}

	UClass* Class = Obj->GetClass();
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("class"), Class->GetPathName());
	Result->SetStringField(TEXT("object"), Obj->GetPathName());

	TArray<TSharedPtr<FJsonValue>> SuperChain;
	for (UClass* Super = Class->GetSuperClass(); Super; Super = Super->GetSuperClass())
	{
		SuperChain.Add(MakeShared<FJsonValueString>(Super->GetPathName()));
	}
	Result->SetArrayField(TEXT("superClasses"), SuperChain);

	TArray<TSharedPtr<FJsonValue>> ObjFlags;
	struct FlagEntry { EObjectFlags Flag; const TCHAR* Name; };
	static const FlagEntry FlagTable[] = {
		{ RF_Public,             TEXT("Public") },
		{ RF_Standalone,         TEXT("Standalone") },
		{ RF_Transactional,      TEXT("Transactional") },
		{ RF_ClassDefaultObject, TEXT("ClassDefaultObject") },
		{ RF_ArchetypeObject,    TEXT("ArchetypeObject") },
		{ RF_Transient,          TEXT("Transient") },
		{ RF_DefaultSubObject,   TEXT("DefaultSubObject") },
		{ RF_WasLoaded,          TEXT("WasLoaded") },
	};
	for (const FlagEntry& Entry : FlagTable)
	{
		if (Obj->HasAnyFlags(Entry.Flag))
		{
			ObjFlags.Add(MakeShared<FJsonValueString>(Entry.Name));
		}
	}
	Result->SetArrayField(TEXT("flags"), ObjFlags);

	TArray<TSharedPtr<FJsonValue>> PropArray;
	for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
		PropObj->SetStringField(TEXT("name"), Prop->GetAuthoredName());
		PropObj->SetStringField(TEXT("type"), GetPropertyTypeString(Prop));
		PropObj->SetBoolField(TEXT("readOnly"), Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly) || !Prop->HasAnyPropertyFlags(CPF_Edit));

		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
		FString ValueStr;
		Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, Obj, PPF_None, nullptr);
		PropObj->SetStringField(TEXT("value"), ValueStr);

		PropArray.Add(MakeShared<FJsonValueObject>(PropObj));
	}
	Result->SetArrayField(TEXT("properties"), PropArray);

	TArray<TSharedPtr<FJsonValue>> FuncArray;
	for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		UFunction* Func = *It;
		TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
		FuncObj->SetStringField(TEXT("name"), Func->GetName());
		FuncObj->SetBoolField(TEXT("isStatic"), Func->HasAnyFunctionFlags(FUNC_Static));
		FuncObj->SetBoolField(TEXT("isPure"), Func->HasAnyFunctionFlags(FUNC_BlueprintPure));

		TArray<TSharedPtr<FJsonValue>> ParamsArray;
		for (TFieldIterator<FProperty> PropIt(Func); PropIt && PropIt->HasAnyPropertyFlags(CPF_Parm); ++PropIt)
		{
			TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
			ParamObj->SetStringField(TEXT("name"), PropIt->GetAuthoredName());
			ParamObj->SetStringField(TEXT("type"), GetPropertyTypeString(*PropIt));
			ParamObj->SetBoolField(TEXT("isReturn"), PropIt->HasAnyPropertyFlags(CPF_ReturnParm));
			ParamObj->SetBoolField(TEXT("isOut"), PropIt->HasAnyPropertyFlags(CPF_OutParm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm));
			ParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
		}
		FuncObj->SetArrayField(TEXT("params"), ParamsArray);
		FuncArray.Add(MakeShared<FJsonValueObject>(FuncObj));
	}
	Result->SetArrayField(TEXT("functions"), FuncArray);

	return MakeSuccessResponse(FString(), Result);
}

TSharedPtr<FJsonObject> FUCPFunctionInvoker::DescribeProperty(
	const FString& ObjectPath,
	const FString& PropertyName)
{
	FString Error;
	UObject* Obj = FindTargetObject(ObjectPath, Error);
	if (!Obj)
	{
		return MakeErrorResponse(FString(), Error);
	}

	FProperty* Prop = FUCPParamConverter::FindPropertyByNameFlexible(Obj->GetClass(), PropertyName);
	if (!Prop)
	{
		return MakeErrorResponse(FString(),
			FString::Printf(TEXT("Property not found: %s on %s"), *PropertyName, *Obj->GetClass()->GetPathName()));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("name"), Prop->GetAuthoredName());
	Result->SetStringField(TEXT("type"), GetPropertyTypeString(Prop));
	Result->SetStringField(TEXT("class"), Obj->GetClass()->GetPathName());
	Result->SetBoolField(TEXT("readOnly"), Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly) || !Prop->HasAnyPropertyFlags(CPF_Edit));

	TArray<TSharedPtr<FJsonValue>> PropFlags;
	struct PropFlagEntry { uint64 Flag; const TCHAR* Name; };
	static const PropFlagEntry PropFlagTable[] = {
		{ CPF_Edit,                TEXT("Edit") },
		{ CPF_BlueprintVisible,    TEXT("BlueprintVisible") },
		{ CPF_BlueprintReadOnly,   TEXT("BlueprintReadOnly") },
		{ CPF_EditConst,           TEXT("EditConst") },
		{ CPF_DisableEditOnInstance,TEXT("DisableEditOnInstance") },
		{ CPF_DisableEditOnTemplate,TEXT("DisableEditOnTemplate") },
		{ CPF_Transient,           TEXT("Transient") },
		{ CPF_Config,              TEXT("Config") },
		{ CPF_GlobalConfig,        TEXT("GlobalConfig") },
		{ CPF_SaveGame,            TEXT("SaveGame") },
		{ CPF_ExposeOnSpawn,       TEXT("ExposeOnSpawn") },
		{ CPF_Interp,              TEXT("Interp") },
		{ CPF_Net,                 TEXT("Net") },
		{ CPF_RepNotify,           TEXT("RepNotify") },
	};
	for (const PropFlagEntry& Entry : PropFlagTable)
	{
		if (Prop->HasAnyPropertyFlags(Entry.Flag))
		{
			PropFlags.Add(MakeShared<FJsonValueString>(Entry.Name));
		}
	}
	Result->SetArrayField(TEXT("flags"), PropFlags);

	if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		Result->SetStringField(TEXT("structType"), StructProp->Struct->GetPathName());
	}
	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
	{
		Result->SetStringField(TEXT("objectClass"), ObjProp->PropertyClass->GetPathName());
	}
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
	{
		Result->SetStringField(TEXT("innerType"), GetPropertyTypeString(ArrayProp->Inner));
	}

	const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
	TSharedPtr<FJsonValue> CurrentVal = FUCPParamConverter::PropertyToJsonValue(Prop, ValuePtr);
	if (CurrentVal.IsValid())
	{
		Result->SetField(TEXT("currentValue"), CurrentVal);
	}

	return MakeSuccessResponse(FString(), Result);
}

TSharedPtr<FJsonObject> FUCPFunctionInvoker::DescribeFunction(
	const FString& ObjectPath,
	const FString& FunctionName)
{
	FString Error;
	UObject* Obj = FindTargetObject(ObjectPath, Error);
	if (!Obj)
	{
		return MakeErrorResponse(FString(), Error);
	}

	UFunction* Func = FindTargetFunction(Obj, FunctionName, Error);
	if (!Func)
	{
		return MakeErrorResponse(FString(), Error);
	}

	TSharedPtr<FJsonObject> FuncDesc = MakeShared<FJsonObject>();
	FuncDesc->SetStringField(TEXT("name"), Func->GetName());
	FuncDesc->SetStringField(TEXT("class"), Obj->GetClass()->GetPathName());

	TArray<TSharedPtr<FJsonValue>> FuncFlags;
	struct FuncFlagEntry { EFunctionFlags Flag; const TCHAR* Name; };
	static const FuncFlagEntry FuncFlagTable[] = {
		{ FUNC_Static,            TEXT("Static") },
		{ FUNC_BlueprintCallable, TEXT("BlueprintCallable") },
		{ FUNC_BlueprintPure,     TEXT("BlueprintPure") },
		{ FUNC_BlueprintEvent,    TEXT("BlueprintEvent") },
		{ FUNC_BlueprintAuthorityOnly, TEXT("BlueprintAuthorityOnly") },
		{ FUNC_Net,               TEXT("Net") },
		{ FUNC_NetServer,         TEXT("NetServer") },
		{ FUNC_NetClient,         TEXT("NetClient") },
		{ FUNC_NetMulticast,      TEXT("NetMulticast") },
		{ FUNC_Native,            TEXT("Native") },
		{ FUNC_Event,             TEXT("Event") },
		{ FUNC_Const,             TEXT("Const") },
		{ FUNC_Exec,              TEXT("Exec") },
		{ FUNC_HasDefaults,       TEXT("HasDefaults") },
	};
	for (const FuncFlagEntry& Entry : FuncFlagTable)
	{
		if (Func->HasAnyFunctionFlags(Entry.Flag))
		{
			FuncFlags.Add(MakeShared<FJsonValueString>(Entry.Name));
		}
	}
	FuncDesc->SetArrayField(TEXT("flags"), FuncFlags);

	TArray<TSharedPtr<FJsonValue>> ParamsArray;
	for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
		ParamObj->SetStringField(TEXT("name"), It->GetAuthoredName());
		ParamObj->SetStringField(TEXT("type"), GetPropertyTypeString(*It));
		ParamObj->SetBoolField(TEXT("isReturn"), It->HasAnyPropertyFlags(CPF_ReturnParm));
		ParamObj->SetBoolField(TEXT("isOut"), It->HasAnyPropertyFlags(CPF_OutParm) && !It->HasAnyPropertyFlags(CPF_ReturnParm));
		ParamObj->SetBoolField(TEXT("isRef"), It->HasAnyPropertyFlags(CPF_ReferenceParm));

		if (Func->HasMetaData(TEXT("WorldContext")))
		{
			FString WcParam = Func->GetMetaData(TEXT("WorldContext"));
			if (It->GetAuthoredName() == WcParam)
			{
				ParamObj->SetBoolField(TEXT("autoWorldContext"), true);
			}
		}

		ParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
	}
	FuncDesc->SetArrayField(TEXT("params"), ParamsArray);

	return MakeSuccessResponse(FString(), FuncDesc);
}

UObject* FUCPFunctionInvoker::FindTargetObject(const FString& ObjectPath, FString& OutError)
{
	if (ObjectPath.IsEmpty())
	{
		OutError = TEXT("Object path is empty");
		return nullptr;
	}

	UObject* Obj = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
	if (Obj)
	{
		return Obj;
	}

	int32 ColonIdx = INDEX_NONE;
	if (ObjectPath.FindChar(TEXT(':'), ColonIdx))
	{
		FString ParentPath = ObjectPath.Left(ColonIdx);
		FString SubPath = ObjectPath.Mid(ColonIdx + 1);

		FString ParentError;
		UObject* ParentObj = FindTargetObject(ParentPath, ParentError);
		if (ParentObj)
		{
			Obj = StaticFindObject(UObject::StaticClass(), ParentObj, *SubPath);
			if (Obj)
			{
				return Obj;
			}

			TArray<UObject*> SubObjects;
			ParentObj->GetDefaultSubobjects(SubObjects);
			for (UObject* Sub : SubObjects)
			{
				if (Sub && Sub->GetName() == SubPath)
				{
					return Sub;
				}
			}

			OutError = FString::Printf(TEXT("SubObject not found: '%s' on %s"), *SubPath, *ParentObj->GetPathName());
			return nullptr;
		}
	}

	int32 DotIdx = INDEX_NONE;
	if (ObjectPath.FindLastChar(TEXT('.'), DotIdx))
	{
		FString ObjectName = ObjectPath.Mid(DotIdx + 1);
		if (ObjectName.StartsWith(TEXT("Default__")))
		{
			FString PackagePath = ObjectPath.Left(DotIdx);

			int32 LastSlash = INDEX_NONE;
			if (PackagePath.FindLastChar(TEXT('/'), LastSlash))
			{
				FString ModuleName = PackagePath.Mid(LastSlash + 1);
				if (!FModuleManager::Get().IsModuleLoaded(FName(*ModuleName)))
				{
					FModuleManager::Get().LoadModule(FName(*ModuleName));
				}
			}

			Obj = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
			if (Obj)
			{
				return Obj;
			}

			OutError = FString::Printf(TEXT("CDO not found (module may not exist): %s"), *ObjectPath);
			return nullptr;
		}
	}

	Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
	if (!Obj)
	{
		OutError = FString::Printf(TEXT("Object not found: %s"), *ObjectPath);
	}
	return Obj;
}

UFunction* FUCPFunctionInvoker::FindTargetFunction(UObject* Obj, const FString& FuncName, FString& OutError)
{
	if (!Obj)
	{
		OutError = TEXT("Null object");
		return nullptr;
	}

	UFunction* Func = Obj->FindFunction(FName(*FuncName));
	if (!Func)
	{
		OutError = FString::Printf(TEXT("Function not found: %s on %s"), *FuncName, *Obj->GetClass()->GetPathName());
	}
	return Func;
}

TSharedPtr<FJsonObject> FUCPFunctionInvoker::MakeErrorResponse(const FString& Id, const FString& Error)
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

TSharedPtr<FJsonObject> FUCPFunctionInvoker::MakeSuccessResponse(const FString& Id, const TSharedPtr<FJsonObject>& Result)
{
	TSharedPtr<FJsonObject> Resp = MakeShared<FJsonObject>();
	if (!Id.IsEmpty())
	{
		Resp->SetStringField(TEXT("id"), Id);
	}
	Resp->SetBoolField(TEXT("success"), true);
	if (Result.IsValid())
	{
		Resp->SetObjectField(TEXT("result"), Result);
	}
	return Resp;
}

FString FUCPFunctionInvoker::GetPropertyTypeString(FProperty* Prop)
{
	if (!Prop)
	{
		return TEXT("unknown");
	}

	if (CastField<FBoolProperty>(Prop))            return TEXT("bool");
	if (CastField<FIntProperty>(Prop))              return TEXT("int32");
	if (CastField<FInt64Property>(Prop))            return TEXT("int64");
	if (CastField<FFloatProperty>(Prop))            return TEXT("float");
	if (CastField<FDoubleProperty>(Prop))           return TEXT("double");
	if (CastField<FStrProperty>(Prop))              return TEXT("FString");
	if (CastField<FNameProperty>(Prop))             return TEXT("FName");
	if (CastField<FTextProperty>(Prop))             return TEXT("FText");
	if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		if (ByteProp->Enum)
			return FString::Printf(TEXT("TEnumAsByte<%s>"), *ByteProp->Enum->GetName());
		return TEXT("uint8");
	}
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		return EnumProp->GetEnum()->GetName();
	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		return FString::Printf(TEXT("%s*"), *ObjProp->PropertyClass->GetName());
	if (FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
		return FString::Printf(TEXT("TSubclassOf<%s>"), *ClassProp->MetaClass->GetName());
	if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop))
		return FString::Printf(TEXT("TSoftObjectPtr<%s>"), *SoftProp->PropertyClass->GetName());
	if (FWeakObjectProperty* WeakProp = CastField<FWeakObjectProperty>(Prop))
		return FString::Printf(TEXT("TWeakObjectPtr<%s>"), *WeakProp->PropertyClass->GetName());
	if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		return StructProp->Struct->GetName();
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		return FString::Printf(TEXT("TArray<%s>"), *GetPropertyTypeString(ArrayProp->Inner));
	if (FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		return FString::Printf(TEXT("TMap<%s,%s>"), *GetPropertyTypeString(MapProp->KeyProp), *GetPropertyTypeString(MapProp->ValueProp));
	if (FSetProperty* SetProp = CastField<FSetProperty>(Prop))
		return FString::Printf(TEXT("TSet<%s>"), *GetPropertyTypeString(SetProp->ElementProp));
	if (CastField<FDelegateProperty>(Prop))         return TEXT("FDelegate");
	if (CastField<FMulticastDelegateProperty>(Prop)) return TEXT("FMulticastDelegate");

	return Prop->GetCPPType();
}
