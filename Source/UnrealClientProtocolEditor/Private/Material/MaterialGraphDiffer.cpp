// MIT License - Copyright (c) 2025 Italink

#include "Material/MaterialGraphDiffer.h"
#include "NodeCode/NodeCodeClassCache.h"
#include "Material/MaterialGraphSerializer.h"
#include "Material/IMaterialPropertyHandler.h"
#include "NodeCode/NodeCodeTextFormat.h"
#include "NodeCode/NodeCodePropertyUtils.h"

#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionRerouteBase.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSwitch.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialAttributeDefinitionMap.h"
#include "MaterialEditingLibrary.h"
#include "ShaderCompiler.h"
#include "UObject/UnrealType.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "IMaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"

DEFINE_LOG_CATEGORY_STATIC(LogUCPMaterial, Log, All);

struct FMatLink
{
	UMaterialExpression* FromExpr = nullptr;
	int32 OutputIndex = 0;
	FExpressionInput* ToInput = nullptr;

	bool operator==(const FMatLink& Other) const
	{
		return FromExpr == Other.FromExpr && OutputIndex == Other.OutputIndex && ToInput == Other.ToInput;
	}
	friend uint32 GetTypeHash(const FMatLink& L)
	{
		return HashCombine(HashCombine(::GetTypeHash(L.FromExpr), ::GetTypeHash(L.OutputIndex)), ::GetTypeHash(L.ToInput));
	}
};

// ---- Entry Points ----

FNodeCodeDiffResult FMaterialGraphDiffer::ApplyMaterial(UMaterial* Material, const FNodeCodeGraphIR& NewIR)
{
	if (!Material)
	{
		UE_LOG(LogUCPMaterial, Error, TEXT("ApplyMaterial: Material is null"));
		return {};
	}
	return DiffAndApply(Material, nullptr, FString(), NewIR);
}

FNodeCodeDiffResult FMaterialGraphDiffer::ApplyComposite(UMaterial* Material, const FString& CompositeName, const FNodeCodeGraphIR& NewIR)
{
	if (!Material)
	{
		UE_LOG(LogUCPMaterial, Error, TEXT("ApplyComposite: Material is null"));
		return {};
	}
	return DiffAndApply(Material, nullptr, CompositeName, NewIR);
}

FNodeCodeDiffResult FMaterialGraphDiffer::ApplyFunction(UMaterialFunction* MaterialFunction, const FNodeCodeGraphIR& NewIR)
{
	if (!MaterialFunction)
	{
		UE_LOG(LogUCPMaterial, Error, TEXT("ApplyFunction: MaterialFunction is null"));
		return {};
	}
	return DiffAndApply(nullptr, MaterialFunction, FString(), NewIR);
}

FNodeCodeDiffResult FMaterialGraphDiffer::ApplyMaterialProperties(UMaterial* Material, const TMap<FString, FString>& Properties)
{
	FNodeCodeDiffResult Result;
	if (!Material)
	{
		return Result;
	}

	FScopedTransaction Transaction(NSLOCTEXT("UCPMaterialGraph", "WriteProperties", "UCP: Write Material Properties"));

	UClass* MatClass = Material->GetClass();
	Material->PreEditChange(nullptr);

	for (const auto& Pair : Properties)
	{
		FProperty* Prop = MatClass->FindPropertyByName(FName(*Pair.Key));
		if (!Prop)
		{
			Result.NodesModified.Add(FString::Printf(TEXT("Property not found: %s"), *Pair.Key));
			continue;
		}

		void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Material);
		FString ImportValue = Pair.Value;
		if (ImportValue.StartsWith(TEXT("\"")) && ImportValue.EndsWith(TEXT("\"")))
		{
			ImportValue = ImportValue.Mid(1, ImportValue.Len() - 2);
			ImportValue = ImportValue.ReplaceEscapedCharWithChar();
		}

		const TCHAR* Buffer = *ImportValue;
		if (Prop->ImportText_Direct(Buffer, ValuePtr, Material, PPF_None))
		{
			Result.NodesModified.Add(FString::Printf(TEXT("%s = %s"), *Pair.Key, *Pair.Value));
		}
		else
		{
			Result.NodesModified.Add(FString::Printf(TEXT("Failed to set %s"), *Pair.Key));
		}
	}

	FPropertyChangedEvent ChangedEvent(nullptr);
	Material->PostEditChangeProperty(ChangedEvent);

	return Result;
}

// ---- Node Matching ----

void FMaterialGraphDiffer::MatchNodes(
	const FNodeCodeGraphIR& OldIR,
	const FNodeCodeGraphIR& NewIR,
	TMap<int32, int32>& OutNewToOld)
{
	TMap<FGuid, int32> OldGuidMap;
	for (int32 i = 0; i < OldIR.Nodes.Num(); ++i)
	{
		if (OldIR.Nodes[i].Guid.IsValid())
		{
			OldGuidMap.Add(OldIR.Nodes[i].Guid, i);
		}
	}

	TSet<int32> MatchedOld;

	// Pass 1: GUID
	for (int32 NewIdx = 0; NewIdx < NewIR.Nodes.Num(); ++NewIdx)
	{
		const FNodeCodeNodeIR& NewNode = NewIR.Nodes[NewIdx];
		if (NewNode.Guid.IsValid())
		{
			if (int32* OldIdx = OldGuidMap.Find(NewNode.Guid))
			{
				if (!MatchedOld.Contains(*OldIdx))
				{
					OutNewToOld.Add(NewIdx, *OldIdx);
					MatchedOld.Add(*OldIdx);
				}
			}
		}
	}

	// Pass 2: ClassName + Properties fingerprint
	for (int32 NewIdx = 0; NewIdx < NewIR.Nodes.Num(); ++NewIdx)
	{
		if (OutNewToOld.Contains(NewIdx))
		{
			continue;
		}

		const FNodeCodeNodeIR& NewNode = NewIR.Nodes[NewIdx];

		for (int32 OldIdx = 0; OldIdx < OldIR.Nodes.Num(); ++OldIdx)
		{
			if (MatchedOld.Contains(OldIdx))
			{
				continue;
			}

			const FNodeCodeNodeIR& OldNode = OldIR.Nodes[OldIdx];
			if (OldNode.ClassName != NewNode.ClassName)
			{
				continue;
			}

			bool bPropsMatch = true;
			for (const auto& Pair : NewNode.Properties)
			{
				const FString* OldVal = OldNode.Properties.Find(Pair.Key);
				if (OldVal && *OldVal != Pair.Value)
				{
					bPropsMatch = false;
					break;
				}
			}

			if (bPropsMatch)
			{
				OutNewToOld.Add(NewIdx, OldIdx);
				MatchedOld.Add(OldIdx);
				break;
			}
		}
	}

}

// ---- Apply Properties ----

void FMaterialGraphDiffer::ApplyPropertyChanges(
	UMaterialExpression* Expression,
	const TMap<FString, FString>& NewProperties,
	TArray<FString>& OutChanges)
{
	FMaterialPropertyHandlerRegistry::Get().ApplySpecial(Expression, NewProperties, OutChanges);

	UClass* ExprClass = Expression->GetClass();

	for (const auto& Pair : NewProperties)
	{
		if (FMaterialPropertyHandlerRegistry::Get().IsHandledKey(Expression, Pair.Key))
		{
			continue;
		}

		FProperty* Prop = ExprClass->FindPropertyByName(FName(*Pair.Key));
		if (!Prop)
		{
			OutChanges.Add(FString::Printf(TEXT("Property not found: %s"), *Pair.Key));
			continue;
		}

		void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Expression);

		FString OldValue;
		Prop->ExportTextItem_Direct(OldValue, ValuePtr, nullptr, Expression, PPF_None, nullptr);

		FString ImportValue = Pair.Value;
		if (ImportValue.StartsWith(TEXT("\"")) && ImportValue.EndsWith(TEXT("\"")))
		{
			ImportValue = ImportValue.Mid(1, ImportValue.Len() - 2);
			ImportValue = ImportValue.ReplaceEscapedCharWithChar();
		}

		const TCHAR* Buffer = *ImportValue;
		const TCHAR* Result = Prop->ImportText_Direct(Buffer, ValuePtr, Expression, PPF_None, GWarn);

		if (Result)
		{
			FString NewValue;
			Prop->ExportTextItem_Direct(NewValue, ValuePtr, nullptr, Expression, PPF_None, nullptr);
			if (OldValue != NewValue)
			{
				OutChanges.Add(FString::Printf(TEXT("%s: %s -> %s"), *Pair.Key, *OldValue, *NewValue));
			}
		}
		else
		{
			OutChanges.Add(FString::Printf(TEXT("Failed to import %s = %s"), *Pair.Key, *Pair.Value));
		}
	}
}

EMaterialProperty FMaterialGraphDiffer::FindMaterialPropertyByName(UMaterial* Material, const FString& Name)
{
	for (int32 i = 0; i < MP_MAX; ++i)
	{
		EMaterialProperty Prop = static_cast<EMaterialProperty>(i);
		const FString& AttrName = FMaterialAttributeDefinitionMap::GetAttributeName(Prop);
		if (AttrName == Name)
		{
			return Prop;
		}
	}
	return MP_MAX;
}

// ---- Diff & Apply ----

FNodeCodeDiffResult FMaterialGraphDiffer::DiffAndApply(
	UMaterial* Material,
	UMaterialFunction* MaterialFunction,
	const FString& CompositeName,
	FNodeCodeGraphIR NewIR)
{
	FNodeCodeDiffResult Result;

	TArray<FString> ValidationWarnings;
	FNodeCodeTextFormat::ValidateGraph(NewIR, ValidationWarnings);
	for (const FString& W : ValidationWarnings)
	{
		UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: %s"), *W);
	}

	FNodeCodeGraphIR OldIR;
	if (Material)
	{
		if (CompositeName.IsEmpty())
		{
			OldIR = FMaterialGraphSerializer::BuildIR(Material);
		}
		else
		{
			OldIR = FMaterialGraphSerializer::BuildCompositeIR(Material, CompositeName);
		}
	}
	else if (MaterialFunction)
	{
		OldIR = FMaterialGraphSerializer::BuildIR(MaterialFunction);
	}

	TMap<int32, int32> NewToOld;
	MatchNodes(OldIR, NewIR, NewToOld);

	TSet<int32> MatchedOldIndices;
	for (auto& Pair : NewToOld)
	{
		MatchedOldIndices.Add(Pair.Value);
	}

	FScopedTransaction Transaction(NSLOCTEXT("UCPMaterialGraph", "WriteGraph", "UCP: Write Material Graph"));

	// Phase 1: Delete
	for (int32 OldIdx = 0; OldIdx < OldIR.Nodes.Num(); ++OldIdx)
	{
		if (MatchedOldIndices.Contains(OldIdx))
		{
			continue;
		}

		const FNodeCodeNodeIR& OldNode = OldIR.Nodes[OldIdx];
		UMaterialExpression* OldExpr = Cast<UMaterialExpression>(OldNode.SourceObject);
		if (OldExpr)
		{
			if (Material)
			{
				UMaterialEditingLibrary::DeleteMaterialExpression(Material, OldExpr);
			}
			else if (MaterialFunction)
			{
				UMaterialEditingLibrary::DeleteMaterialExpressionInFunction(MaterialFunction, OldExpr);
			}
			Result.NodesRemoved.Add(FString::Printf(TEXT("N_%s %s"), *OldNode.Index, *OldNode.ClassName));
		}
	}

	// Phase 2: Create
	TMap<FString, UMaterialExpression*> NewIndexToExpr;

	for (int32 NewIdx = 0; NewIdx < NewIR.Nodes.Num(); ++NewIdx)
	{
		const FNodeCodeNodeIR& NewNode = NewIR.Nodes[NewIdx];

		if (int32* OldIdx = NewToOld.Find(NewIdx))
		{
			NewIndexToExpr.Add(NewNode.Index, Cast<UMaterialExpression>(OldIR.Nodes[*OldIdx].SourceObject));
		}
		else
		{
			UClass* ExprClass = FNodeCodeClassCache::Get().FindClass(NewNode.ClassName);
			if (!ExprClass)
			{
				UE_LOG(LogUCPMaterial, Error, TEXT("WriteGraph: Unknown expression class: %s"), *NewNode.ClassName);
				continue;
			}

			UMaterialExpression* NewExpr = nullptr;
			if (Material)
			{
				NewExpr = UMaterialEditingLibrary::CreateMaterialExpression(Material, ExprClass, 0, 0);
			}
			else if (MaterialFunction)
			{
				NewExpr = UMaterialEditingLibrary::CreateMaterialExpressionInFunction(MaterialFunction, ExprClass, 0, 0);
			}

			if (!NewExpr)
			{
				UE_LOG(LogUCPMaterial, Error, TEXT("WriteGraph: Failed to create expression: %s"), *NewNode.ClassName);
				continue;
			}

			TArray<FString> Changes;
			ApplyPropertyChanges(NewExpr, NewNode.Properties, Changes);

			NewIndexToExpr.Add(NewNode.Index, NewExpr);
			Result.NodesAdded.Add(FString::Printf(TEXT("N_%s %s"), *NewNode.Index, *NewNode.ClassName));
		}
	}

	// Phase 3: Update properties
	for (int32 NewIdx = 0; NewIdx < NewIR.Nodes.Num(); ++NewIdx)
	{
		int32* OldIdx = NewToOld.Find(NewIdx);
		if (!OldIdx)
		{
			continue;
		}

		const FNodeCodeNodeIR& NewNode = NewIR.Nodes[NewIdx];
		const FNodeCodeNodeIR& OldNode = OldIR.Nodes[*OldIdx];
		UMaterialExpression* Expr = Cast<UMaterialExpression>(OldNode.SourceObject);

		if (!Expr)
		{
			continue;
		}

		bool bPropsChanged = (NewNode.Properties.Num() != OldNode.Properties.Num());
		if (!bPropsChanged)
		{
			for (const auto& Pair : NewNode.Properties)
			{
				const FString* OldVal = OldNode.Properties.Find(Pair.Key);
				if (!OldVal || *OldVal != Pair.Value)
				{
					bPropsChanged = true;
					break;
				}
			}
		}

		if (bPropsChanged)
		{
			Expr->PreEditChange(nullptr);

			TArray<FString> Changes;
			ApplyPropertyChanges(Expr, NewNode.Properties, Changes);

			FPropertyChangedEvent ChangedEvent(nullptr);
			Expr->PostEditChangeProperty(ChangedEvent);

			if (Changes.Num() > 0)
			{
				Result.NodesModified.Add(FString::Printf(TEXT("N_%s: %s"),
					*NewNode.Index, *FString::Join(Changes, TEXT("; "))));
			}
		}
	}

	// Phase 4+5: Connection diff
	auto FindInputIndexByName = [](UMaterialExpression* Expr, const FString& InputName) -> int32
	{
		if (InputName.IsEmpty())
		{
			return (Expr->GetInput(0) != nullptr) ? 0 : INDEX_NONE;
		}

		for (int32 i = 0; ; ++i)
		{
			FExpressionInput* Input = Expr->GetInput(i);
			if (!Input) break;
			if (Expr->GetInputName(i).ToString() == InputName)
			{
				return i;
			}
		}

		UClass* ExprClass = Expr->GetClass();
		for (TFieldIterator<FStructProperty> PropIt(ExprClass); PropIt; ++PropIt)
		{
			FStructProperty* StructProp = *PropIt;
			if (!StructProp || !StructProp->Struct) continue;

			const UStruct* Current = StructProp->Struct;
			bool bIsInput = false;
			while (Current)
			{
				if (Current->GetFName() == NAME_ExpressionInput) { bIsInput = true; break; }
				Current = Current->GetSuperStruct();
			}

			if (bIsInput && StructProp->GetName() == InputName)
			{
				const void* InputPtr = StructProp->ContainerPtrToValuePtr<void>(Expr);
				for (int32 i = 0; ; ++i)
				{
					FExpressionInput* Input = Expr->GetInput(i);
					if (!Input) break;
					if (Input == InputPtr) return i;
				}
			}
		}

		// Fallback: if only one input exists, accept any name
		int32 InputCount = 0;
		for (int32 i = 0; ; ++i)
		{
			if (!Expr->GetInput(i)) break;
			InputCount++;
		}
		if (InputCount == 1)
		{
			return 0;
		}

		return INDEX_NONE;
	};

	auto FindOutputIndexByName = [](UMaterialExpression* Expr, const FString& OutputName) -> int32
	{
		if (OutputName.IsEmpty()) return 0;

		TArray<FExpressionOutput>& Outputs = Expr->GetOutputs();
		for (int32 i = 0; i < Outputs.Num(); ++i)
		{
			const FExpressionOutput& Output = Outputs[i];
			if (!Output.OutputName.IsNone() && Output.OutputName.ToString() == OutputName) return i;
			if (Output.MaskR && !Output.MaskG && !Output.MaskB && !Output.MaskA && OutputName == TEXT("R")) return i;
			if (!Output.MaskR && Output.MaskG && !Output.MaskB && !Output.MaskA && OutputName == TEXT("G")) return i;
			if (!Output.MaskR && !Output.MaskG && Output.MaskB && !Output.MaskA && OutputName == TEXT("B")) return i;
			if (!Output.MaskR && !Output.MaskG && !Output.MaskB && Output.MaskA && OutputName == TEXT("A")) return i;
			if (Output.MaskR && Output.MaskG && Output.MaskB && !Output.MaskA && OutputName == TEXT("RGB")) return i;
			if (Output.MaskR && Output.MaskG && Output.MaskB && Output.MaskA && OutputName == TEXT("RGBA")) return i;
		}
		return INDEX_NONE;
	};

	TSet<FMatLink> DesiredLinks;
	TArray<FMatLink> LinksToCreate;

	for (const FNodeCodeLinkIR& Link : NewIR.Links)
	{
		UMaterialExpression** FromExprPtr = NewIndexToExpr.Find(Link.FromNodeIndex);
		if (!FromExprPtr || !*FromExprPtr)
		{
			UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: Link source N_%s not found"), *Link.FromNodeIndex);
			continue;
		}

		int32 FromOutputIndex = FindOutputIndexByName(*FromExprPtr, Link.FromOutputName);
		if (FromOutputIndex == INDEX_NONE)
		{
			UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: Output '%s' not found on N_%s"), *Link.FromOutputName, *Link.FromNodeIndex);
			continue;
		}

		FExpressionInput* TargetInput = nullptr;

		if (Link.bToGraphOutput && Material)
		{
			EMaterialProperty MatProp = FindMaterialPropertyByName(Material, Link.ToInputName);
			if (MatProp != MP_MAX)
			{
				TargetInput = Material->GetExpressionInputForProperty(MatProp);
			}
			else
			{
				UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: Unknown material output: %s"), *Link.ToInputName);
			}
		}
		else if (!Link.bToGraphOutput)
		{
			UMaterialExpression** ToExprPtr = NewIndexToExpr.Find(Link.ToNodeIndex);
			if (!ToExprPtr || !*ToExprPtr)
			{
				UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: Link target N_%s not found"), *Link.ToNodeIndex);
				continue;
			}

			int32 ToInputIndex = FindInputIndexByName(*ToExprPtr, Link.ToInputName);
			if (ToInputIndex == INDEX_NONE)
			{
				FString AvailableInputs;
				for (int32 DbgIdx = 0; ; ++DbgIdx)
				{
					FExpressionInput* DbgInput = (*ToExprPtr)->GetInput(DbgIdx);
					if (!DbgInput) break;
					if (!AvailableInputs.IsEmpty()) AvailableInputs += TEXT(", ");
					AvailableInputs += (*ToExprPtr)->GetInputName(DbgIdx).ToString();
				}
					UE_LOG(LogUCPMaterial, Warning, TEXT("WriteGraph: Input '%s' not found on N_%s (%s). Available: [%s]"),
					*Link.ToInputName, *Link.ToNodeIndex, *(*ToExprPtr)->GetClass()->GetName(), *AvailableInputs);
				continue;
			}

			TargetInput = (*ToExprPtr)->GetInput(ToInputIndex);
		}

		if (!TargetInput) continue;

		FMatLink Desired;
		Desired.FromExpr = *FromExprPtr;
		Desired.OutputIndex = FromOutputIndex;
		Desired.ToInput = TargetInput;

		DesiredLinks.Add(Desired);

		bool bAlreadyConnected = (TargetInput->Expression == *FromExprPtr && TargetInput->OutputIndex == FromOutputIndex);
		if (!bAlreadyConnected)
		{
			LinksToCreate.Add(Desired);
		}
	}

	TSet<UMaterialExpression*> ScopeExprSet;
	for (auto& Pair : NewIndexToExpr)
	{
		if (Pair.Value) ScopeExprSet.Add(Pair.Value);
	}

	for (auto& Pair : NewIndexToExpr)
	{
		UMaterialExpression* Expr = Pair.Value;
		if (!Expr) continue;

		for (FExpressionInputIterator It(Expr); It; ++It)
		{
			if (!It->Expression || !ScopeExprSet.Contains(It->Expression)) continue;

			FMatLink Live;
			Live.FromExpr = It->Expression;
			Live.OutputIndex = It->OutputIndex;
			Live.ToInput = It.Input;

			if (!DesiredLinks.Contains(Live))
			{
				Result.LinksRemoved.Add(FString::Printf(TEXT("N_?->N_%s.%s"), *Pair.Key, *Expr->GetInputName(It.Index).ToString()));
				It->Expression = nullptr;
			}
		}
	}

	if (Material && CompositeName.IsEmpty())
	{
		for (int32 i = 0; i < MP_MAX; ++i)
		{
			EMaterialProperty Prop = static_cast<EMaterialProperty>(i);
			FExpressionInput* Input = Material->GetExpressionInputForProperty(Prop);
			if (!Input || !Input->Expression || !ScopeExprSet.Contains(Input->Expression)) continue;

			FMatLink Live;
			Live.FromExpr = Input->Expression;
			Live.OutputIndex = Input->OutputIndex;
			Live.ToInput = Input;

			if (!DesiredLinks.Contains(Live))
			{
				const FString& AttrName = FMaterialAttributeDefinitionMap::GetAttributeName(Prop);
				Result.LinksRemoved.Add(FString::Printf(TEXT("N?->[%s]"), *AttrName));
				Input->Expression = nullptr;
			}
		}
	}

	for (const FMatLink& Link : LinksToCreate)
	{
		Link.FromExpr->ConnectExpression(Link.ToInput, Link.OutputIndex);
		Result.LinksAdded.Add(FString::Printf(TEXT("%s[%d] -> input"),
			*FNodeCodeClassCache::Get().GetSerializableName(Link.FromExpr->GetClass()),
			Link.OutputIndex));
	}

	// Phase 6: Post-apply
	if (Material)
	{
		if (Material->MaterialGraph)
		{
			Material->MaterialGraph->RebuildGraph();
		}

		{
			TSet<UMaterialExpression*> ReachableExprs;
			FMaterialGraphSerializer::CollectAllConnectedNodes(Material, ReachableExprs);

			TArray<UMaterialExpression*> OrphanedExprs;
			for (const auto& Expr : Material->GetExpressions())
			{
				if (!Expr) continue;
				if (Cast<UMaterialExpressionComment>(Expr.Get())) continue;
				if (!ReachableExprs.Contains(Expr.Get()))
				{
					OrphanedExprs.Add(Expr.Get());
				}
			}

			for (UMaterialExpression* Expr : OrphanedExprs)
			{
				Result.NodesRemoved.Add(FString::Printf(TEXT("(orphaned) %s"),
					*FNodeCodeClassCache::Get().GetSerializableName(Expr->GetClass())));
				Material->GetExpressionCollection().RemoveExpression(Expr);
				Material->RemoveExpressionParameter(Expr);
				Expr->MarkAsGarbage();
			}

			if (OrphanedExprs.Num() > 0 && Material->MaterialGraph)
			{
				Material->MaterialGraph->RebuildGraph();
			}
		}

		bool bGraphChanged = Result.NodesAdded.Num() > 0 || Result.NodesRemoved.Num() > 0
			|| Result.LinksAdded.Num() > 0 || Result.LinksRemoved.Num() > 0;

		if (bGraphChanged)
		{
			UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
		}
		UMaterialEditingLibrary::RecompileMaterial(Material);
		GShaderCompilingManager->FinishAllCompilation();
#if ENGINE_MINOR_VERSION >= 7
		const FMaterialResource* MatResource = Material->GetMaterialResource(GMaxRHIShaderPlatform);
#else
		const FMaterialResource* MatResource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
#endif
		if (MatResource)
		{
			const TArray<FString>& CompileErrors = MatResource->GetCompileErrors();
			const TArray<UMaterialExpression*>& FailExprs = MatResource->GetErrorExpressions();
			for (int32 ErrIdx = 0; ErrIdx < CompileErrors.Num(); ++ErrIdx)
			{
				FString Entry;
				if (ErrIdx < FailExprs.Num() && FailExprs[ErrIdx])
				{
					FString NodeId = NodeCodeUtils::GuidToBase62(FailExprs[ErrIdx]->MaterialExpressionGuid);
					FString ClassName = FNodeCodeClassCache::Get().GetSerializableName(FailExprs[ErrIdx]->GetClass());
					Entry = FString::Printf(TEXT("N_%s (%s): %s"), *NodeId, *ClassName, *CompileErrors[ErrIdx]);
				}
				else
				{
					Entry = CompileErrors[ErrIdx];
				}
				Result.CompileErrors.Add(Entry);
			}
		}

		if (GEditor)
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			IAssetEditorInstance* EditorInstance = AssetEditorSubsystem ? AssetEditorSubsystem->FindEditorForAsset(Material, false) : nullptr;
			if (EditorInstance)
			{
				IMaterialEditor* MaterialEditor = static_cast<IMaterialEditor*>(EditorInstance);
				UMaterialInterface* EditorMaterial = MaterialEditor->GetMaterialInterface();
				if (EditorMaterial && EditorMaterial != Material)
				{
					UMaterial* PreviewMaterial = Cast<UMaterial>(EditorMaterial);
					if (PreviewMaterial)
					{
						PreviewMaterial->GetExpressionCollection().Empty();
						for (UMaterialExpression* Expr : Material->GetExpressions())
						{
							UMaterialExpression* DupExpr = Cast<UMaterialExpression>(StaticDuplicateObject(Expr, PreviewMaterial));
							if (DupExpr)
							{
								DupExpr->Material = PreviewMaterial;
								PreviewMaterial->GetExpressionCollection().AddExpression(DupExpr);
							}
						}

						for (int32 i = 0; i < MP_MAX; ++i)
						{
							EMaterialProperty Prop = static_cast<EMaterialProperty>(i);
							FExpressionInput* OrigInput = Material->GetExpressionInputForProperty(Prop);
							FExpressionInput* PreviewInput = PreviewMaterial->GetExpressionInputForProperty(Prop);
							if (OrigInput && PreviewInput)
							{
								if (OrigInput->Expression)
								{
									int32 OrigIdx = Material->GetExpressions().IndexOfByKey(OrigInput->Expression);
									if (OrigIdx != INDEX_NONE && OrigIdx < PreviewMaterial->GetExpressions().Num())
									{
										PreviewInput->Expression = PreviewMaterial->GetExpressions()[OrigIdx];
										PreviewInput->OutputIndex = OrigInput->OutputIndex;
										PreviewInput->Mask = OrigInput->Mask;
										PreviewInput->MaskR = OrigInput->MaskR;
										PreviewInput->MaskG = OrigInput->MaskG;
										PreviewInput->MaskB = OrigInput->MaskB;
										PreviewInput->MaskA = OrigInput->MaskA;
									}
								}
								else
								{
									PreviewInput->Expression = nullptr;
								}
							}
						}

						for (int32 i = 0; i < PreviewMaterial->GetExpressions().Num(); ++i)
						{
							UMaterialExpression* PreviewExpr = PreviewMaterial->GetExpressions()[i];
							for (FExpressionInputIterator It(PreviewExpr); It; ++It)
							{
								if (It->Expression)
								{
									int32 OrigIdx = Material->GetExpressions().IndexOfByKey(It->Expression);
									if (OrigIdx != INDEX_NONE && OrigIdx < PreviewMaterial->GetExpressions().Num())
									{
										It->Expression = PreviewMaterial->GetExpressions()[OrigIdx];
									}
									else
									{
										It->Expression = nullptr;
									}
								}
							}
						}

						if (PreviewMaterial->MaterialGraph)
						{
							PreviewMaterial->MaterialGraph->RebuildGraph();
						}

						MaterialEditor->UpdateMaterialAfterGraphChange();
					}
				}
			}
		}
	}
	else if (MaterialFunction)
	{
		UMaterialEditingLibrary::LayoutMaterialFunctionExpressions(MaterialFunction);
		UMaterialEditingLibrary::UpdateMaterialFunction(MaterialFunction, nullptr);
	}

	return Result;
}
