// MIT License - Copyright (c) 2025 Italink

#include "UCPParamConverter.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/PropertyAccessUtil.h"
#include "JsonObjectConverter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

FString FUCPParamConverter::JsonToParams(
	UFunction* Function,
	const TSharedPtr<FJsonObject>& ParamsJson,
	uint8* ParamBuffer,
	UObject* ContextObject)
{
	if (!Function || !ParamBuffer)
	{
		return TEXT("Invalid function or param buffer");
	}

	FString WorldContextParamName;
	if (Function->HasMetaData(TEXT("WorldContext")))
	{
		WorldContextParamName = Function->GetMetaData(TEXT("WorldContext"));
	}

	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		FString ParamName = Prop->GetAuthoredName();
		void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(ParamBuffer);

		if (!WorldContextParamName.IsEmpty() && ParamName == WorldContextParamName)
		{
			if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
			{
				UWorld* World = GetBestWorld();
				if (World)
				{
					ObjProp->SetObjectPropertyValue(ValuePtr, World);
				}
			}
			continue;
		}

		if (!ParamsJson.IsValid() || !ParamsJson->HasField(ParamName))
		{
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				continue;
			}
			continue;
		}

		TSharedPtr<FJsonValue> JsonVal = ParamsJson->TryGetField(ParamName);
		if (!JsonVal.IsValid())
		{
			continue;
		}

		FString Error = JsonValueToProperty(JsonVal, Prop, ValuePtr);
		if (!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Param '%s': %s"), *ParamName, *Error);
		}
	}

	return FString();
}

TSharedPtr<FJsonObject> FUCPParamConverter::ParamsToJson(
	UFunction* Function,
	const uint8* ParamBuffer)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	if (!Function || !ParamBuffer)
	{
		return Result;
	}

	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm) || Prop->HasAnyPropertyFlags(CPF_OutParm))
		{
			const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(ParamBuffer);
			TSharedPtr<FJsonValue> JsonVal = PropertyToJsonValue(Prop, ValuePtr);
			if (JsonVal.IsValid())
			{
				Result->SetField(Prop->GetAuthoredName(), JsonVal);
			}
		}
	}

	return Result;
}

TSharedPtr<FJsonValue> FUCPParamConverter::PropertyToJsonValue(
	FProperty* Property,
	const void* ValuePtr)
{
	if (!Property || !ValuePtr)
	{
		return MakeShared<FJsonValueNull>();
	}

	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (Obj)
		{
			return MakeShared<FJsonValueString>(Obj->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}

	if (FWeakObjectProperty* WeakProp = CastField<FWeakObjectProperty>(Property))
	{
		UObject* Obj = WeakProp->GetObjectPropertyValue(ValuePtr);
		if (Obj)
		{
			return MakeShared<FJsonValueString>(Obj->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}

	if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPtr& SoftPtr = *reinterpret_cast<const FSoftObjectPtr*>(ValuePtr);
		return MakeShared<FJsonValueString>(SoftPtr.ToSoftObjectPath().ToString());
	}

	if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		UObject* Obj = ClassProp->GetObjectPropertyValue(ValuePtr);
		if (UClass* AsClass = Cast<UClass>(Obj))
		{
			return MakeShared<FJsonValueString>(AsClass->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}

	if (CastField<FDelegateProperty>(Property) || CastField<FMulticastDelegateProperty>(Property))
	{
		return MakeShared<FJsonValueString>(TEXT("<delegate:unsupported>"));
	}

	return FJsonObjectConverter::UPropertyToJsonValue(Property, ValuePtr);
}

FString FUCPParamConverter::JsonValueToProperty(
	const TSharedPtr<FJsonValue>& JsonVal,
	FProperty* Property,
	void* ValuePtr)
{
	if (!JsonVal.IsValid() || !Property || !ValuePtr)
	{
		return TEXT("Null value, property, or pointer");
	}

	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		if (JsonVal->IsNull())
		{
			ObjProp->SetObjectPropertyValue(ValuePtr, nullptr);
			return FString();
		}
		FString ObjPath = JsonVal->AsString();
		if (ObjPath.IsEmpty() || ObjPath == TEXT("null"))
		{
			ObjProp->SetObjectPropertyValue(ValuePtr, nullptr);
			return FString();
		}

		UObject* Obj = ResolveObjectPath(ObjPath, ObjProp->PropertyClass);
		if (!Obj)
		{
			return FString::Printf(TEXT("Could not resolve object path: %s"), *ObjPath);
		}
		ObjProp->SetObjectPropertyValue(ValuePtr, Obj);
		return FString();
	}

	if (FWeakObjectProperty* WeakProp = CastField<FWeakObjectProperty>(Property))
	{
		if (JsonVal->IsNull())
		{
			WeakProp->SetObjectPropertyValue(ValuePtr, nullptr);
			return FString();
		}
		FString ObjPath = JsonVal->AsString();
		UObject* Obj = ObjPath.IsEmpty() ? nullptr : ResolveObjectPath(ObjPath, WeakProp->PropertyClass);
		WeakProp->SetObjectPropertyValue(ValuePtr, Obj);
		return FString();
	}

	if (CastField<FSoftObjectProperty>(Property))
	{
		FSoftObjectPtr& SoftPtr = *reinterpret_cast<FSoftObjectPtr*>(ValuePtr);
		if (JsonVal->IsNull())
		{
			SoftPtr.Reset();
			return FString();
		}
		FString PathStr = JsonVal->AsString();
		SoftPtr = FSoftObjectPath(PathStr);
		return FString();
	}

	if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		if (JsonVal->IsNull())
		{
			ClassProp->SetObjectPropertyValue(ValuePtr, nullptr);
			return FString();
		}
		FString ClassPath = JsonVal->AsString();
		UClass* LoadedClass = LoadClass<UObject>(nullptr, *ClassPath);
		if (!LoadedClass)
		{
			return FString::Printf(TEXT("Could not load class: %s"), *ClassPath);
		}
		ClassProp->SetObjectPropertyValue(ValuePtr, LoadedClass);
		return FString();
	}

	if (CastField<FDelegateProperty>(Property) || CastField<FMulticastDelegateProperty>(Property))
	{
		return TEXT("Delegate parameters are not supported");
	}

	if (!FJsonObjectConverter::JsonValueToUProperty(JsonVal, Property, ValuePtr))
	{
		return FString::Printf(TEXT("FJsonObjectConverter failed for property type: %s"), *Property->GetCPPType());
	}

	return FString();
}

UObject* FUCPParamConverter::ResolveObjectPath(const FString& Path, UClass* ExpectedClass)
{
	UClass* SearchClass = ExpectedClass ? ExpectedClass : UObject::StaticClass();

	UObject* Obj = StaticFindObject(SearchClass, nullptr, *Path);
	if (!Obj)
	{
		Obj = StaticLoadObject(SearchClass, nullptr, *Path);
	}
	return Obj;
}

UWorld* FUCPParamConverter::GetBestWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			return Context.World();
		}
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Editor && Context.World())
		{
			return Context.World();
		}
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.World())
		{
			return Context.World();
		}
	}
	return nullptr;
}

FProperty* FUCPParamConverter::FindPropertyByNameFlexible(UClass* Class, const FString& PropertyName)
{
	if (!Class)
	{
		return nullptr;
	}

	FProperty* Prop = Class->FindPropertyByName(FName(*PropertyName));
	if (Prop)
	{
		return Prop;
	}

	// UE strips the 'b' prefix from bool properties in reflection (authored name).
	// Try toggling the prefix for flexible lookup.
	if (PropertyName.StartsWith(TEXT("b")) && PropertyName.Len() > 1 && FChar::IsUpper(PropertyName[1]))
	{
		Prop = Class->FindPropertyByName(FName(*PropertyName.Mid(1)));
		if (Prop)
		{
			return Prop;
		}
	}
	else if (PropertyName.Len() > 0 && FChar::IsUpper(PropertyName[0]))
	{
		Prop = Class->FindPropertyByName(FName(*(FString(TEXT("b")) + PropertyName)));
		if (Prop)
		{
			return Prop;
		}
	}

	return nullptr;
}
