// MIT License - Copyright (c) 2025 Italink

#include "Blueprint/BlueprintSectionHandler.h"
#include "Blueprint/BlueprintGraphSerializer.h"
#include "Blueprint/BlueprintGraphDiffer.h"
#include "NodeCode/NodeCodeTextFormat.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "ScopedTransaction.h"

DEFINE_LOG_CATEGORY_STATIC(LogUCPBPHandler, Log, All);

static const FString EventGraphType = TEXT("EventGraph");
static const FString FunctionType = TEXT("Function");
static const FString MacroType = TEXT("Macro");
static const FString VariablesType = TEXT("Variables");

bool FBlueprintSectionHandler::CanHandle(UObject* Asset, const FString& Type) const
{
	if (!Asset || !Asset->IsA<UBlueprint>())
	{
		return false;
	}

	if (Type.IsEmpty())
	{
		return true;
	}

	return Type == EventGraphType || Type == FunctionType || Type == MacroType || Type == VariablesType;
}

TArray<FNodeCodeSectionIR> FBlueprintSectionHandler::ListSections(UObject* Asset)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (!Blueprint)
	{
		return {};
	}

	return FBlueprintGraphSerializer::ListSections(Blueprint);
}

FNodeCodeSectionIR FBlueprintSectionHandler::Read(UObject* Asset, const FString& Type, const FString& Name)
{
	FNodeCodeSectionIR Section;
	Section.Type = Type;
	Section.Name = Name;

	UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (!Blueprint)
	{
		return Section;
	}

	if (Type == VariablesType)
	{
		Section.Properties = ReadVariables(Blueprint);
		return Section;
	}

	UEdGraph* Graph = FBlueprintGraphSerializer::FindGraphBySection(Blueprint, Type, Name);
	if (Graph)
	{
		Section.Graph = FBlueprintGraphSerializer::BuildIR(Graph);
	}
	else
	{
		UE_LOG(LogUCPBPHandler, Warning, TEXT("Read: Graph not found for [%s:%s]"), *Type, *Name);
	}

	return Section;
}

FNodeCodeDiffResult FBlueprintSectionHandler::Write(UObject* Asset, const FNodeCodeSectionIR& Section)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (!Blueprint)
	{
		return {};
	}

	if (Section.Type == VariablesType)
	{
		return WriteVariables(Blueprint, Section.Properties);
	}

	UEdGraph* Graph = FBlueprintGraphSerializer::FindGraphBySection(Blueprint, Section.Type, Section.Name);
	if (!Graph)
	{
		UE_LOG(LogUCPBPHandler, Error, TEXT("Write: Graph not found for [%s:%s]"), *Section.Type, *Section.Name);
		return {};
	}

	return FBlueprintGraphDiffer::Apply(Blueprint, Graph, Section.Graph);
}

bool FBlueprintSectionHandler::CreateSection(UObject* Asset, const FString& Type, const FString& Name)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (!Blueprint)
	{
		return false;
	}

	FString DecodedName = Name.Replace(TEXT("_"), TEXT(" "));

	if (Type == FunctionType)
	{
		UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FName(*DecodedName),
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		if (NewGraph)
		{
			FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, NewGraph, true, nullptr);
			return true;
		}
	}
	else if (Type == MacroType)
	{
		UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FName(*DecodedName),
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		if (NewGraph)
		{
			FBlueprintEditorUtils::AddMacroGraph(Blueprint, NewGraph, true, nullptr);
			return true;
		}
	}

	return false;
}

bool FBlueprintSectionHandler::RemoveSection(UObject* Asset, const FString& Type, const FString& Name)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (!Blueprint)
	{
		return false;
	}

	UEdGraph* Graph = FBlueprintGraphSerializer::FindGraphBySection(Blueprint, Type, Name);
	if (Graph)
	{
		FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
		return true;
	}

	return false;
}

// ---- Variables ----

TMap<FString, FString> FBlueprintSectionHandler::ReadVariables(UBlueprint* Blueprint)
{
	TMap<FString, FString> Result;
	if (!Blueprint)
	{
		return Result;
	}

	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();

		VarObj->SetStringField(TEXT("PinCategory"), Var.VarType.PinCategory.ToString());

		if (!Var.VarType.PinSubCategory.IsNone())
		{
			VarObj->SetStringField(TEXT("PinSubCategory"), Var.VarType.PinSubCategory.ToString());
		}

		if (Var.VarType.PinSubCategoryObject.IsValid())
		{
			VarObj->SetStringField(TEXT("PinSubCategoryObject"), Var.VarType.PinSubCategoryObject->GetPathName());
		}

		if (Var.VarType.ContainerType != EPinContainerType::None)
		{
			FString ContainerStr;
			switch (Var.VarType.ContainerType)
			{
			case EPinContainerType::Array: ContainerStr = TEXT("Array"); break;
			case EPinContainerType::Set: ContainerStr = TEXT("Set"); break;
			case EPinContainerType::Map: ContainerStr = TEXT("Map"); break;
			default: break;
			}
			if (!ContainerStr.IsEmpty())
			{
				VarObj->SetStringField(TEXT("ContainerType"), ContainerStr);
			}
		}

		if (!Var.DefaultValue.IsEmpty())
		{
			VarObj->SetStringField(TEXT("DefaultValue"), Var.DefaultValue);
		}

		if (!Var.Category.EqualTo(UEdGraphSchema_K2::VR_DefaultCategory))
		{
			VarObj->SetStringField(TEXT("Category"), Var.Category.ToString());
		}

		if (Var.PropertyFlags & CPF_Net)
		{
			VarObj->SetBoolField(TEXT("Replicated"), true);
		}

		FString JsonStr;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonStr);
		FJsonSerializer::Serialize(VarObj.ToSharedRef(), Writer);

		Result.Add(Var.VarName.ToString(), JsonStr);
	}

	return Result;
}

static FEdGraphPinType ParsePinTypeFromJson(const TSharedPtr<FJsonObject>& JsonObj)
{
	FEdGraphPinType PinType;
	PinType.PinCategory = FName(*JsonObj->GetStringField(TEXT("PinCategory")));

	if (JsonObj->HasField(TEXT("PinSubCategory")))
	{
		PinType.PinSubCategory = FName(*JsonObj->GetStringField(TEXT("PinSubCategory")));
	}

	if (JsonObj->HasField(TEXT("PinSubCategoryObject")))
	{
		FString ObjPath = JsonObj->GetStringField(TEXT("PinSubCategoryObject"));
		UObject* SubCatObj = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjPath);
		if (SubCatObj)
		{
			PinType.PinSubCategoryObject = SubCatObj;
		}
	}

	if (JsonObj->HasField(TEXT("ContainerType")))
	{
		FString ContainerStr = JsonObj->GetStringField(TEXT("ContainerType"));
		if (ContainerStr == TEXT("Array")) PinType.ContainerType = EPinContainerType::Array;
		else if (ContainerStr == TEXT("Set")) PinType.ContainerType = EPinContainerType::Set;
		else if (ContainerStr == TEXT("Map")) PinType.ContainerType = EPinContainerType::Map;
	}

	return PinType;
}

static bool PinTypesEqual(const FEdGraphPinType& A, const FEdGraphPinType& B)
{
	return A.PinCategory == B.PinCategory
		&& A.PinSubCategory == B.PinSubCategory
		&& A.PinSubCategoryObject == B.PinSubCategoryObject
		&& A.ContainerType == B.ContainerType;
}

FNodeCodeDiffResult FBlueprintSectionHandler::WriteVariables(UBlueprint* Blueprint, const TMap<FString, FString>& Variables)
{
	FNodeCodeDiffResult Result;
	if (!Blueprint)
	{
		return Result;
	}

	FScopedTransaction Transaction(NSLOCTEXT("UCPBlueprint", "WriteVariables", "UCP: Write Blueprint Variables"));

	TSet<FName> DesiredVarNames;

	for (const auto& Pair : Variables)
	{
		FName VarName(*Pair.Key);
		DesiredVarNames.Add(VarName);

		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Pair.Value);
		if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
		{
			Result.NodesModified.Add(FString::Printf(TEXT("Failed to parse JSON for variable: %s"), *Pair.Key));
			continue;
		}

		FEdGraphPinType PinType = ParsePinTypeFromJson(JsonObj);

		FString DefaultValue;
		if (JsonObj->HasField(TEXT("DefaultValue")))
		{
			DefaultValue = JsonObj->GetStringField(TEXT("DefaultValue"));
		}

		FBPVariableDescription* ExistingVar = nullptr;
		for (FBPVariableDescription& Var : Blueprint->NewVariables)
		{
			if (Var.VarName == VarName)
			{
				ExistingVar = &Var;
				break;
			}
		}

		if (!ExistingVar)
		{
			if (FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarName, PinType, DefaultValue))
			{
				Result.NodesAdded.Add(FString::Printf(TEXT("Variable: %s"), *Pair.Key));
			}
			else
			{
				Result.NodesModified.Add(FString::Printf(TEXT("Failed to create variable: %s"), *Pair.Key));
				continue;
			}

			for (FBPVariableDescription& Var : Blueprint->NewVariables)
			{
				if (Var.VarName == VarName)
				{
					ExistingVar = &Var;
					break;
				}
			}
		}
		else
		{
			TArray<FString> Changes;

			if (!PinTypesEqual(ExistingVar->VarType, PinType))
			{
				FBlueprintEditorUtils::ChangeMemberVariableType(Blueprint, VarName, PinType);
				Changes.Add(TEXT("type"));
			}

			if (ExistingVar->DefaultValue != DefaultValue)
			{
				ExistingVar->DefaultValue = DefaultValue;
				Changes.Add(TEXT("default"));
			}

			bool bDesiredReplicated = JsonObj->HasField(TEXT("Replicated")) && JsonObj->GetBoolField(TEXT("Replicated"));
			bool bCurrentReplicated = (ExistingVar->PropertyFlags & CPF_Net) != 0;
			if (bDesiredReplicated != bCurrentReplicated)
			{
				if (bDesiredReplicated)
				{
					ExistingVar->PropertyFlags |= CPF_Net;
				}
				else
				{
					ExistingVar->PropertyFlags &= ~CPF_Net;
				}
				Changes.Add(TEXT("replicated"));
			}

			if (Changes.Num() > 0)
			{
				Result.NodesModified.Add(FString::Printf(TEXT("Variable %s: %s"), *Pair.Key, *FString::Join(Changes, TEXT(", "))));
			}
		}

		if (ExistingVar && JsonObj->HasField(TEXT("Category")))
		{
			FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint, VarName, nullptr,
				FText::FromString(JsonObj->GetStringField(TEXT("Category"))));
		}
	}

	TArray<FName> VarsToRemove;
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		if (!DesiredVarNames.Contains(Var.VarName))
		{
			VarsToRemove.Add(Var.VarName);
		}
	}

	for (const FName& VarName : VarsToRemove)
	{
		FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VarName);
		Result.NodesRemoved.Add(FString::Printf(TEXT("Variable: %s"), *VarName.ToString()));
	}

	return Result;
}
