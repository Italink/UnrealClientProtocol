// MIT License - Copyright (c) 2025 Italink

#include "Material/IMaterialPropertyHandler.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSwitch.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialAttributeDefinitionMap.h"

// ---- Registry ----

FMaterialPropertyHandlerRegistry& FMaterialPropertyHandlerRegistry::Get()
{
	static FMaterialPropertyHandlerRegistry Instance;
	return Instance;
}

void FMaterialPropertyHandlerRegistry::Register(TSharedPtr<IMaterialPropertyHandler> Handler)
{
	if (Handler.IsValid())
	{
		Handlers.Add(Handler);
	}
}

void FMaterialPropertyHandlerRegistry::SerializeSpecial(UMaterialExpression* Expr, TMap<FString, FString>& OutProps) const
{
	for (const auto& Handler : Handlers)
	{
		if (Handler->CanHandle(Expr))
		{
			Handler->Serialize(Expr, OutProps);
		}
	}
}

void FMaterialPropertyHandlerRegistry::ApplySpecial(UMaterialExpression* Expr, const TMap<FString, FString>& Props, TArray<FString>& OutChanges) const
{
	for (const auto& Handler : Handlers)
	{
		if (Handler->CanHandle(Expr))
		{
			Handler->Apply(Expr, Props, OutChanges);
		}
	}
}

bool FMaterialPropertyHandlerRegistry::IsHandledKey(UMaterialExpression* /*Expr*/, const FString& Key) const
{
	static const TSet<FString> SpecialKeys = { TEXT("InputNames"), TEXT("SwitchInputNames"), TEXT("Attributes") };
	return SpecialKeys.Contains(Key);
}

// ---- Custom Expression Handler ----

class FCustomExpressionPropertyHandler : public IMaterialPropertyHandler
{
public:
	virtual bool CanHandle(UMaterialExpression* Expr) const override
	{
		return Cast<UMaterialExpressionCustom>(Expr) != nullptr;
	}

	virtual void Serialize(UMaterialExpression* Expr, TMap<FString, FString>& OutProps) const override
	{
		UMaterialExpressionCustom* CustomExpr = CastChecked<UMaterialExpressionCustom>(Expr);
		if (CustomExpr->Inputs.Num() > 0)
		{
			bool bHasNamedInputs = false;
			for (const FCustomInput& CI : CustomExpr->Inputs)
			{
				if (!CI.InputName.IsNone())
				{
					bHasNamedInputs = true;
					break;
				}
			}

			if (bHasNamedInputs)
			{
				FString InputNames = TEXT("[");
				bool bFirst = true;
				for (const FCustomInput& CI : CustomExpr->Inputs)
				{
					if (!bFirst) InputNames += TEXT(",");
					InputNames += FString::Printf(TEXT("\"%s\""), *CI.InputName.ToString());
					bFirst = false;
				}
				InputNames += TEXT("]");
				OutProps.Add(TEXT("InputNames"), MoveTemp(InputNames));
			}
		}
	}

	virtual void Apply(UMaterialExpression* Expr, const TMap<FString, FString>& Props, TArray<FString>& OutChanges) const override
	{
		UMaterialExpressionCustom* CustomExpr = CastChecked<UMaterialExpressionCustom>(Expr);
		if (const FString* InputNamesStr = Props.Find(TEXT("InputNames")))
		{
			FString Working = *InputNamesStr;
			Working.TrimStartAndEndInline();
			if (Working.StartsWith(TEXT("[")) && Working.EndsWith(TEXT("]")))
			{
				Working = Working.Mid(1, Working.Len() - 2);
			}

			TArray<FString> Names;
			Working.ParseIntoArray(Names, TEXT(","));
			for (FString& Name : Names)
			{
				Name.TrimStartAndEndInline();
				if (Name.StartsWith(TEXT("\"")) && Name.EndsWith(TEXT("\"")))
				{
					Name = Name.Mid(1, Name.Len() - 2);
				}
			}

			bool bNamesMatch = (Names.Num() == CustomExpr->Inputs.Num());
			if (bNamesMatch)
			{
				for (int32 i = 0; i < Names.Num(); ++i)
				{
					if (CustomExpr->Inputs[i].InputName != FName(*Names[i]))
					{
						bNamesMatch = false;
						break;
					}
				}
			}

			if (!bNamesMatch)
			{
				CustomExpr->Inputs.Empty();
				for (FString& Name : Names)
				{
					FCustomInput NewInput;
					NewInput.InputName = FName(*Name);
					CustomExpr->Inputs.Add(MoveTemp(NewInput));
				}

				if (CustomExpr->Inputs.Num() == 0)
				{
					FCustomInput DefaultInput;
					DefaultInput.InputName = NAME_None;
					CustomExpr->Inputs.Add(MoveTemp(DefaultInput));
				}

				CustomExpr->RebuildOutputs();
				OutChanges.Add(FString::Printf(TEXT("InputNames: %d inputs"), CustomExpr->Inputs.Num()));
			}
		}
	}
};

// ---- Switch Expression Handler ----

class FSwitchExpressionPropertyHandler : public IMaterialPropertyHandler
{
public:
	virtual bool CanHandle(UMaterialExpression* Expr) const override
	{
		return Cast<UMaterialExpressionSwitch>(Expr) != nullptr;
	}

	virtual void Serialize(UMaterialExpression* Expr, TMap<FString, FString>& OutProps) const override
	{
		UMaterialExpressionSwitch* SwitchExpr = CastChecked<UMaterialExpressionSwitch>(Expr);
		if (SwitchExpr->Inputs.Num() > 0)
		{
			FString SwitchNames = TEXT("[");
			bool bFirst = true;
			for (const FSwitchCustomInput& SI : SwitchExpr->Inputs)
			{
				if (!bFirst) SwitchNames += TEXT(",");
				SwitchNames += FString::Printf(TEXT("\"%s\""), *SI.InputName.ToString());
				bFirst = false;
			}
			SwitchNames += TEXT("]");
			OutProps.Add(TEXT("SwitchInputNames"), MoveTemp(SwitchNames));
		}
	}

	virtual void Apply(UMaterialExpression* Expr, const TMap<FString, FString>& Props, TArray<FString>& OutChanges) const override
	{
		UMaterialExpressionSwitch* SwitchExpr = CastChecked<UMaterialExpressionSwitch>(Expr);
		if (const FString* SwitchNamesStr = Props.Find(TEXT("SwitchInputNames")))
		{
			FString Working = *SwitchNamesStr;
			Working.TrimStartAndEndInline();
			if (Working.StartsWith(TEXT("[")) && Working.EndsWith(TEXT("]")))
			{
				Working = Working.Mid(1, Working.Len() - 2);
			}

			TArray<FString> Names;
			Working.ParseIntoArray(Names, TEXT(","));

			SwitchExpr->Inputs.Empty();
			for (FString& Name : Names)
			{
				Name.TrimStartAndEndInline();
				if (Name.StartsWith(TEXT("\"")) && Name.EndsWith(TEXT("\"")))
				{
					Name = Name.Mid(1, Name.Len() - 2);
				}
				FSwitchCustomInput NewInput;
				NewInput.InputName = FName(*Name);
				SwitchExpr->Inputs.Add(MoveTemp(NewInput));
			}

			OutChanges.Add(FString::Printf(TEXT("SwitchInputNames: %d cases"), SwitchExpr->Inputs.Num()));
		}
	}
};

// ---- SetMaterialAttributes Handler ----

class FSetMaterialAttributesPropertyHandler : public IMaterialPropertyHandler
{
public:
	virtual bool CanHandle(UMaterialExpression* Expr) const override
	{
		return Cast<UMaterialExpressionSetMaterialAttributes>(Expr) != nullptr;
	}

	virtual void Serialize(UMaterialExpression* Expr, TMap<FString, FString>& OutProps) const override
	{
		UMaterialExpressionSetMaterialAttributes* SetAttrExpr = CastChecked<UMaterialExpressionSetMaterialAttributes>(Expr);
		if (SetAttrExpr->AttributeSetTypes.Num() > 0)
		{
			FString Attributes = TEXT("[");
			bool bFirst = true;
			for (const FGuid& AttrGuid : SetAttrExpr->AttributeSetTypes)
			{
				if (!bFirst) Attributes += TEXT(",");
				const FString& AttrName = FMaterialAttributeDefinitionMap::GetAttributeName(AttrGuid);
				Attributes += FString::Printf(TEXT("\"%s\""), *AttrName);
				bFirst = false;
			}
			Attributes += TEXT("]");
			OutProps.Add(TEXT("Attributes"), MoveTemp(Attributes));
		}
	}

	virtual void Apply(UMaterialExpression* Expr, const TMap<FString, FString>& Props, TArray<FString>& OutChanges) const override
	{
		UMaterialExpressionSetMaterialAttributes* SetAttrExpr = CastChecked<UMaterialExpressionSetMaterialAttributes>(Expr);
		if (const FString* AttrsStr = Props.Find(TEXT("Attributes")))
		{
			FString Working = *AttrsStr;
			Working.TrimStartAndEndInline();
			if (Working.StartsWith(TEXT("[")) && Working.EndsWith(TEXT("]")))
			{
				Working = Working.Mid(1, Working.Len() - 2);
			}

			TArray<FString> AttrNames;
			Working.ParseIntoArray(AttrNames, TEXT(","));

			SetAttrExpr->AttributeSetTypes.Empty();
			SetAttrExpr->Inputs.Empty();
			SetAttrExpr->Inputs.Add(FExpressionInput());

			for (FString& AttrName : AttrNames)
			{
				AttrName.TrimStartAndEndInline();
				if (AttrName.StartsWith(TEXT("\"")) && AttrName.EndsWith(TEXT("\"")))
				{
					AttrName = AttrName.Mid(1, AttrName.Len() - 2);
				}

				FGuid AttrGuid;
				for (int32 i = 0; i < MP_MAX; ++i)
				{
					EMaterialProperty Prop = static_cast<EMaterialProperty>(i);
					const FString& Name = FMaterialAttributeDefinitionMap::GetAttributeName(Prop);
					if (Name == AttrName)
					{
						AttrGuid = FMaterialAttributeDefinitionMap::GetID(Prop);
						break;
					}
				}

				if (!AttrGuid.IsValid())
				{
					AttrGuid = FMaterialAttributeDefinitionMap::GetCustomAttributeID(AttrName);
				}

				if (AttrGuid.IsValid())
				{
					SetAttrExpr->AttributeSetTypes.Add(AttrGuid);
					SetAttrExpr->Inputs.Add(FExpressionInput());
				}
				else
				{
					OutChanges.Add(FString::Printf(TEXT("Unknown attribute: %s"), *AttrName));
				}
			}

			OutChanges.Add(FString::Printf(TEXT("Attributes: %d attributes"), SetAttrExpr->AttributeSetTypes.Num()));
		}
	}
};

// ---- Auto-registration ----

struct FMaterialPropertyHandlerAutoRegister
{
	FMaterialPropertyHandlerAutoRegister()
	{
		auto& Registry = FMaterialPropertyHandlerRegistry::Get();
		Registry.Register(MakeShared<FCustomExpressionPropertyHandler>());
		Registry.Register(MakeShared<FSwitchExpressionPropertyHandler>());
		Registry.Register(MakeShared<FSetMaterialAttributesPropertyHandler>());
	}
};

static FMaterialPropertyHandlerAutoRegister GAutoRegisterMaterialPropertyHandlers;
